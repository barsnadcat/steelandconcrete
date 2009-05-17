#include "QuickGUIManager.h"
#include "QuickGUISerialReader.h"
#include "QuickGUIRoot.h"
#include "QuickGUIFactoryManager.h"
#include "QuickGUIWidgetFactory.h"
#include "QuickGUISheet.h"
#include "QuickGUISheetManager.h"
#include "QuickGUITimerManager.h"
#include "QuickGUIContextMenu.h"

#include "OgreSceneManager.h"
#include "OgreViewport.h"
#include "OgreRenderQueue.h"

namespace QuickGUI
{
	GUIManagerDesc::GUIManagerDesc()
	{
		name = "";
		viewport = NULL;
		sceneManager = NULL;
		scrollLastClicked = false;
		queueID = Ogre::RENDER_QUEUE_OVERLAY;
		determineClickEvents = true;
		clickTime = 90;
		doubleClickTime = 400;
		tripleClickTime = 400;

		// by default, we support codepoints 9, and 32-166.
		supportedCodePoints.insert(9);
		for(Ogre::UTFString::code_point i = 32; i < 167; ++i)
			supportedCodePoints.insert(i);
	}

	GUIManager::GUIManager(GUIManagerDesc& d) :
		mDraggingWidget(false),
		mActiveSheet(0),
		mButtonMask(0),
		mWidgetUnderMouseCursor(0),
		mLastClickedWidget(0),
		mDownOnBorder(false),
		mKeyModifiers(0),
		mPreviousBorder(BORDER_NONE),
		mResizableBorder(BORDER_NONE),
		mViewportWidth(0),
		mViewportHeight(0)
	{
		mGUIManagerDesc.name = d.name;

		mBrush = Brush::getSingletonPtr();

		// If the scenemanager argument is NULL, try to use the first available scene manager
		if(d.sceneManager == NULL)
		{
			Ogre::SceneManagerEnumerator::SceneManagerIterator it = Ogre::Root::getSingleton().getSceneManagerIterator();
			if(it.hasMoreElements())
				setSceneManager(it.getNext());
		}
		else
			setSceneManager(d.sceneManager);

		// If the viewport argument is NULL, use the first available viewport, from the scene manager chosen above.
		if(d.viewport == NULL)
		{
			if(mGUIManagerDesc.sceneManager != NULL)
			{
				Ogre::SceneManager::CameraIterator it = mGUIManagerDesc.sceneManager->getCameraIterator();
				if(it.hasMoreElements())
					setViewport(it.getNext()->getViewport());
			}
		}
		else
			setViewport(d.viewport);

		setSupportedCodePoints(d.supportedCodePoints);

		d.mouseCursorDesc.guiManager = this;
		mMouseCursor = OGRE_NEW_T(MouseCursor,Ogre::MEMCATEGORY_GENERAL)(d.mouseCursorDesc);
		_clearMouseTrackingData();

		mTimer = OGRE_NEW Ogre::Timer();
		
		TimerDesc td;
		td.repeat = false;
		td.timePeriod = 3;
		mHoverTimer = TimerManager::getSingleton().createTimer(td);
		mHoverTimer->setCallback(&GUIManager::hoverTimerCallback,this);

		setRenderQueueID(d.queueID);
	}

	GUIManager::~GUIManager()
	{
		// Clear out freelist
		while(!mFreeList.empty())
		{
			FactoryManager::getSingleton().getWidgetFactory()->destroyInstance(mFreeList.front());
			mFreeList.pop_front();
		}

		TimerManager::getSingleton().destroyTimer(mHoverTimer);

		OGRE_DELETE mTimer;

		OGRE_DELETE_T(mMouseCursor,MouseCursor,Ogre::MEMCATEGORY_GENERAL);
	}

	void GUIManager::_clearMouseTrackingData()
	{
		for(int i = 0; i < NUM_MOUSE_BUTTONS; ++i)
		{
			mMouseButtonDown[i] = NULL;
		}
	}

	void GUIManager::checkIfCursorOverResizableBorder(Point position)
	{
		// Change cursor if we're over a resizable widget's border, or restore cursor if we have left resizable widget's border
		if(mWidgetUnderMouseCursor != NULL)
		{
			mPreviousBorder = mResizableBorder;
			mResizableBorder = mWidgetUnderMouseCursor->getBorderSide(position);

			switch(mResizableBorder)
			{
				case BORDER_LEFT:
					if(mWidgetUnderMouseCursor->getResizeFromLeft())
						mMouseCursor->_setSkinType("hresize");
					else mResizableBorder = BORDER_NONE;
					break;
				case BORDER_RIGHT:
					if(mWidgetUnderMouseCursor->getResizeFromRight())
						mMouseCursor->_setSkinType("hresize");
					else mResizableBorder = BORDER_NONE;
					break;
				case BORDER_TOP:
					if(mWidgetUnderMouseCursor->getResizeFromTop())
						mMouseCursor->_setSkinType("vresize");
					else mResizableBorder = BORDER_NONE;
					break;
				case BORDER_BOTTOM:
					if(mWidgetUnderMouseCursor->getResizeFromBottom())
						mMouseCursor->_setSkinType("vresize");
					else mResizableBorder = BORDER_NONE;
					break;
				case BORDER_TOP_LEFT:
					if(mWidgetUnderMouseCursor->getResizeFromLeft() && mWidgetUnderMouseCursor->getResizeFromTop())
						mMouseCursor->_setSkinType("diag1resize");
					else mResizableBorder = BORDER_NONE;
					break;
				case BORDER_BOTTOM_RIGHT:
					if(mWidgetUnderMouseCursor->getResizeFromRight() && mWidgetUnderMouseCursor->getResizeFromBottom())
						mMouseCursor->_setSkinType("diag1resize");
					else mResizableBorder = BORDER_NONE;
					break;
				case BORDER_TOP_RIGHT:
					if(mWidgetUnderMouseCursor->getResizeFromRight() && mWidgetUnderMouseCursor->getResizeFromTop())
						mMouseCursor->_setSkinType("diag2resize");
					else mResizableBorder = BORDER_NONE;
					break;
				case BORDER_BOTTOM_LEFT:
					if(mWidgetUnderMouseCursor->getResizeFromLeft() && mWidgetUnderMouseCursor->getResizeFromBottom())
						mMouseCursor->_setSkinType("diag2resize");
					else mResizableBorder = BORDER_NONE;
					break;
				case BORDER_NONE:
					// Revert cursor if we have just moved off a resizable border
					if(mPreviousBorder != BORDER_NONE)
						mMouseCursor->_setSkinType(mMouseCursor->mDefaultSkinType);
					break;
			}
		}
	}

	void GUIManager::clearWidgetUnderMouseCursorReference()
	{
		mWidgetUnderMouseCursor = NULL;

		// Make sure cursor is the default normal one
		mMouseCursor->_setSkinType(mMouseCursor->mDefaultSkinType);
	}

	void GUIManager::clearLastClickedWidgetReference()
	{
		mLastClickedWidget = NULL;
	}

	void GUIManager::draw()
	{
		SheetManager::getSingleton().cleanup();

		// Clear out freelist
		while(!mFreeList.empty())
		{
			FactoryManager::getSingleton().getWidgetFactory()->destroyInstance(mFreeList.front());
			mFreeList.pop_front();
		}

		mBrush->updateSceneManager(mGUIManagerDesc.sceneManager);
		mBrush->updateViewport(mGUIManagerDesc.viewport);
		mBrush->setRenderTarget(mGUIManagerDesc.viewport);

		mBrush->prepareToDraw();

		if(mActiveSheet != NULL)
		{
			mActiveSheet->cleanupWidgets();
			mActiveSheet->draw();
		}

		mMouseCursor->draw();

		mBrush->restore();
	}

	Sheet* GUIManager::getActiveSheet()
	{
		return mActiveSheet;
	}

	Widget* GUIManager::getLastClickedWidget()
	{
		return mLastClickedWidget;
	}

	MouseCursor* GUIManager::getMouseCursor()
	{
		return mMouseCursor;
	}

	Ogre::String GUIManager::getName()
	{
		return mGUIManagerDesc.name;
	}

	bool GUIManager::getScrollLastClicked()
	{
		return mGUIManagerDesc.scrollLastClicked;
	}

	Widget* GUIManager::getWidgetUnderMouseCursor()
	{
		return mWidgetUnderMouseCursor;
	}

	Ogre::Viewport* GUIManager::getViewport()
	{
		return mGUIManagerDesc.viewport;
	}

	bool GUIManager::injectChar(Ogre::UTFString::unicode_char c)
	{
		// do nothing if no active sheet is in use
		if(mActiveSheet == NULL)
			return false;

		if(!isSupportedCodePoint(c))
			return false;

		Widget* w = mActiveSheet->getKeyboardListener();

		if((w == NULL) || !(w->getEnabled()))
			return false;

		KeyEventArgs args(w);
		args.keyModifiers = mKeyModifiers;
		args.keyMask = mButtonMask;
		args.codepoint = c;

		w->fireWidgetEvent(WIDGET_EVENT_CHARACTER_KEY,args);

		return true;
	}

	bool GUIManager::injectKeyDown(const KeyCode& kc)
	{
		// do nothing if no active sheet is in use
		if(mActiveSheet == NULL)
			return false;

		// Turn on modifier
		if( (kc == KC_LCONTROL) || (kc == KC_RCONTROL) )
			mKeyModifiers |= CTRL;
		else if( (kc == KC_LSHIFT) || (kc == KC_RSHIFT) )
			mKeyModifiers |= SHIFT;
		else if( (kc == KC_LMENU) || (kc == KC_RMENU) )
			mKeyModifiers |= ALT;

		Widget* w = mActiveSheet->getKeyboardListener();

		if((w == NULL) || !(w->getEnabled()))
			return false;

		KeyEventArgs args(w);
		args.scancode = kc;
		args.keyMask = mButtonMask;
		args.keyModifiers = mKeyModifiers;

		w->fireWidgetEvent(WIDGET_EVENT_KEY_DOWN,args);

		return true;
	}

	bool GUIManager::injectKeyUp(const KeyCode& kc)
	{
		// do nothing if no active sheet is in use
		if(mActiveSheet == NULL)
			return false;

		//Turn off modifier
		if( (kc == KC_LCONTROL) || (kc == KC_RCONTROL) )
			mKeyModifiers &= ~CTRL;
		else if( (kc == KC_LSHIFT) || (kc == KC_RSHIFT) )
			mKeyModifiers &= ~SHIFT;
		else if( (kc == KC_LMENU) || (kc == KC_RMENU) )
			mKeyModifiers &= ~ALT;

		Widget* w = mActiveSheet->getKeyboardListener();

		if((w == NULL) || !(w->getEnabled()))
			return false;

		KeyEventArgs args(w);
		args.scancode = kc;
		args.keyModifiers = mKeyModifiers;

		w->fireWidgetEvent(WIDGET_EVENT_KEY_UP,args);

		return true;
	}

	bool GUIManager::injectMouseButtonDown(const MouseButtonID& button)
	{
		// do nothing if no active sheet is in use
		if(mActiveSheet == NULL)
			return false;

		// If the mouse is disabled, we do not accept this injection of input
		if( !mMouseCursor->getEnabled() ) 
			return false;

		// Users can manually inject single/double/triple click input, or have it detected
		// from mouse button up/down injections.
		if(mGUIManagerDesc.determineClickEvents)
		{
			// Record time of MouseDown, for single/double/triple click determination
			mTimeOfButtonDown[button] = mTimer->getMilliseconds();

			// Check if time since last double click is within triple click time
			if((mTimeOfButtonDown[button] - mTimeOfDoubleClick[button]) <= mGUIManagerDesc.tripleClickTime)
			{
				// We have passed the criteria for a triple click injection, but we have to 
				// make sure the mouse button is going down on the same widget for all clicks.
				if(mMouseButtonDown[button] == mWidgetUnderMouseCursor)
					return injectMouseTripleClick(button);
			}
			// Check if time since last click is within double click time
			if((mTimeOfButtonDown[button] - mTimeOfClick[button]) <= mGUIManagerDesc.doubleClickTime)
			{
				// We have passed the criteria for a double click injection, but we have to 
				// make sure the mouse button is going down on the same widget for both clicks.
				if(mMouseButtonDown[button] == mWidgetUnderMouseCursor)
					return injectMouseDoubleClick(button);
			}
		}

		// If we make it here, a simple mouse button down has occurred.

		// Modify the button mask
		mButtonMask |= (1 << button);

		// Record that the mouse button went down on this widget
		mMouseButtonDown[button] = mWidgetUnderMouseCursor;
		mLastClickedWidget = mWidgetUnderMouseCursor;

		// Check if Sheet's keyboard listener (widget receiving keyboard events) needs to be updated.
		if((mLastClickedWidget != NULL) && (mLastClickedWidget->getConsumeKeyboardEvents()))
			mActiveSheet->setKeyboardListener(mLastClickedWidget);

		// Create MouseEventArgs, for use with any fired events
		MouseEventArgs args(mWidgetUnderMouseCursor);
		args.position = mMouseCursor->getPosition();
		args.button = button;
		args.buttonMask = mButtonMask;
		args.keyModifiers = mKeyModifiers;

		// Get the Window under the mouse cursor.  If it is not the Sheet, but a child Window,
		// make sure it has focus. (bring to front)
		Window* win = mActiveSheet->findWindowAtPoint(args.position);
		if(win != mActiveSheet->getWindowInFocus())
			// FOCUS_GAINED and FOCUS_LOST events will be fired if appropriate.
			mActiveSheet->focusWindow(win);

		if(mWidgetUnderMouseCursor != NULL)
		{
			if(button == MB_Left)
			{
				mWidgetUnderMouseCursor->setGrabbed(true);

				// If Widget is grabable, change cursor
				if(mWidgetUnderMouseCursor->getDragable())
					mMouseCursor->_setSkinType("grabbed");
			}

			// Fire EVENT_MOUSE_BUTTON_DOWN event to the widget in focus
			mWidgetUnderMouseCursor->fireWidgetEvent(WIDGET_EVENT_MOUSE_BUTTON_DOWN,args);

			// Record mouse down on border only if the widget supports resizing for that border, and LMB is down
			mDownOnBorder = false;
			if((win != NULL) && (button == MB_Left))
			{
				switch(mWidgetUnderMouseCursor->getBorderSide(mMouseCursor->getPosition() - win->getPosition()))
				{
				case BORDER_LEFT:
					if(mWidgetUnderMouseCursor->getResizeFromLeft())
						mDownOnBorder = true;
					break;
				case BORDER_RIGHT:
					if(mWidgetUnderMouseCursor->getResizeFromRight())
						mDownOnBorder = true;
					break;
				case BORDER_TOP:
					if(mWidgetUnderMouseCursor->getResizeFromTop())
						mDownOnBorder = true;
					break;
				case BORDER_BOTTOM:
					if(mWidgetUnderMouseCursor->getResizeFromBottom())
						mDownOnBorder = true;
					break;
				case BORDER_TOP_LEFT:
					if(mWidgetUnderMouseCursor->getResizeFromLeft() && mWidgetUnderMouseCursor->getResizeFromTop())
						mDownOnBorder = true;
					break;
				case BORDER_BOTTOM_RIGHT:
					if(mWidgetUnderMouseCursor->getResizeFromRight() && mWidgetUnderMouseCursor->getResizeFromBottom())
						mDownOnBorder = true;
					break;
				case BORDER_TOP_RIGHT:
					if(mWidgetUnderMouseCursor->getResizeFromRight() && mWidgetUnderMouseCursor->getResizeFromTop())
						mDownOnBorder = true;
					break;
				case BORDER_BOTTOM_LEFT:
					if(mWidgetUnderMouseCursor->getResizeFromLeft() && mWidgetUnderMouseCursor->getResizeFromBottom())
						mDownOnBorder = true;
					break;
				}
			}
		}

		return (mWidgetUnderMouseCursor != NULL);
	}

	bool GUIManager::injectMouseButtonUp(const MouseButtonID& button)
	{
		// do nothing if no active sheet is in use
		if(mActiveSheet == NULL)
			return false;

		// If the mouse is disabled, we do not accept this injection of input
		if( !mMouseCursor->getEnabled() ) 
			return false;

		// Modify the button mask
		mButtonMask &= !(1 << button);

		// If the mouse button goes up and is not over the same widget
		// the mouse button went down on, disregard this injection.
		if( mMouseButtonDown[button] != mWidgetUnderMouseCursor )
			return (mWidgetUnderMouseCursor != NULL);

		// after this point, we know that the user had mouse button down on this widget, and is now doing mouse button up on the same widget.

		// Create MouseEventArgs, for use with any fired events
		MouseEventArgs args(mWidgetUnderMouseCursor);
		args.position = mMouseCursor->getPosition();
		args.button = button;
		args.buttonMask = mButtonMask;
		args.keyModifiers = mKeyModifiers;

		if(args.widget != NULL)
		{
			// Check if a widget is currently being dragged.
			if(args.widget->getGrabbed())
			{
				args.widget->setGrabbed(false);

				if(args.widget->getDragable())
				{
					// Set cursor to grabable
					mMouseCursor->_setSkinType("grabable");
				}
				else
				{
					// Restore cursor to default
					mMouseCursor->_setSkinType(mMouseCursor->mDefaultSkinType);
				}

				// We do not want the widget to receive an EVENT_MOUSE_BUTTON_UP event if we are dropping
				// the widget.  Think of Diablo II, dragging a potion to your belt.  If we sent the mouse
				// button up event the potion would be drank as soon as you dropped it into the belt.
				if(args.widget->getBeingDragged())
				{
					args.widget->fireWidgetEvent(WIDGET_EVENT_DROPPED,args);
					return (mWidgetUnderMouseCursor != NULL);
				}
			}

			// Fire EVENT_MOUSE_BUTTON_UP event
			args.widget->fireWidgetEvent(WIDGET_EVENT_MOUSE_BUTTON_UP,args);

			// Show context menu
			if((mActiveSheet != NULL) && (button == MB_Right) && (args.widget->getContextMenuName() != ""))
				mActiveSheet->getContextMenu(args.widget->getContextMenuName())->show(args.position);

			// Users can manually inject single/double/triple click input, or have it detected
			// from mouse button up/down injections.
			if(mGUIManagerDesc.determineClickEvents)
			{
				if((mTimer->getMilliseconds() - mTimeOfButtonDown[button]) <= mGUIManagerDesc.clickTime)
				{
					injectMouseClick(button);
				}
			}
		}

		return (mWidgetUnderMouseCursor != NULL);
	}

	bool GUIManager::injectMouseClick(const MouseButtonID& button)
	{
		// do nothing if no active sheet is in use
		if(mActiveSheet == NULL)
			return false;

		// If the mouse is disabled, we do not accept this injection of input
		if( !mMouseCursor->getEnabled() ) 
			return false;

		// Modify the button mask
		mButtonMask &= !(1 << button);

		// Record the time the click occurred. Useful for generating double clicks.
		mTimeOfClick[button] = mTimer->getMilliseconds();

		// If the mouse button goes up and is not over the same widget
		// the mouse button went down on, disregard this injection.
		if( mMouseButtonDown[button] != mWidgetUnderMouseCursor )
			return (mWidgetUnderMouseCursor != NULL);

		mLastClickedWidget = mWidgetUnderMouseCursor;

		// Create MouseEventArgs, for use with any fired events
		MouseEventArgs args(mWidgetUnderMouseCursor);
		args.button = button;
		args.position = mMouseCursor->getPosition();
		args.buttonMask = mButtonMask;
		args.keyModifiers = mKeyModifiers;

		if(args.widget != NULL)
			args.widget->fireWidgetEvent(WIDGET_EVENT_MOUSE_CLICK,args);

		return (mWidgetUnderMouseCursor != NULL);
	}

	bool GUIManager::injectMouseDoubleClick(const MouseButtonID& button)
	{
		// do nothing if no active sheet is in use
		if(mActiveSheet == NULL)
			return false;

		// If the mouse is disabled, we do not accept this injection of input
		if( !mMouseCursor->getEnabled() ) 
			return false;

		// Modify the button mask
		mButtonMask &= !(1 << button);

		// Record the time the click occurred. Useful for generating triple clicks.
		mTimeOfDoubleClick[button] = mTimer->getMilliseconds();

		// If the mouse button goes up and is not over the same widget
		// the mouse button went down on, disregard this injection.
		if( mMouseButtonDown[button] != mWidgetUnderMouseCursor )
			return (mWidgetUnderMouseCursor != NULL);

		mLastClickedWidget = mWidgetUnderMouseCursor;

		// Create MouseEventArgs, for use with any fired events
		MouseEventArgs args(mWidgetUnderMouseCursor);
		args.button = button;
		args.position = mMouseCursor->getPosition();
		args.buttonMask = mButtonMask;
		args.keyModifiers = mKeyModifiers;

		if(args.widget != NULL)
			args.widget->fireWidgetEvent(WIDGET_EVENT_MOUSE_CLICK_DOUBLE,args);

		return (mWidgetUnderMouseCursor != NULL);
	}

	bool GUIManager::injectMouseTripleClick(const MouseButtonID& button)
	{
		// do nothing if no active sheet is in use
		if(mActiveSheet == NULL)
			return false;

		// If the mouse is disabled, we do not accept this injection of input
		if( !mMouseCursor->getEnabled() ) 
			return false;

		// Modify the button mask
		mButtonMask &= !(1 << button);

		// If the mouse button goes up and is not over the same widget
		// the mouse button went down on, disregard this injection.
		if( mMouseButtonDown[button] != mWidgetUnderMouseCursor )
			return (mWidgetUnderMouseCursor != NULL);

		mLastClickedWidget = mWidgetUnderMouseCursor;

		// Create MouseEventArgs, for use with any fired events
		MouseEventArgs args(mWidgetUnderMouseCursor);
		args.button = button;
		args.position = mMouseCursor->getPosition();
		args.buttonMask = mButtonMask;
		args.keyModifiers = mKeyModifiers;

		if(args.widget != NULL)
			args.widget->fireWidgetEvent(WIDGET_EVENT_MOUSE_CLICK_TRIPLE,args);

		return (mWidgetUnderMouseCursor != NULL);
	}

	bool GUIManager::injectMouseMove(const int& xPixelOffset, const int& yPixelOffset)
	{
		// do nothing if no active sheet is in use
		if(mActiveSheet == NULL)
			return false;

		// If the mouse is disabled, we do not accept this injection of input
		if( !mMouseCursor->getEnabled() ) 
			return false;

		// Restart the Hover Timer whenever the mouse moves.
		mHoverTimer->start();

		// Get the widget the cursor is over.
		Window* win = mActiveSheet->findWindowAtPoint(mMouseCursor->getPosition());
		Widget* w = win->findWidgetAtPoint(mMouseCursor->getPosition() - win->getPosition());

		// See if we should be dragging or resizing a widget.
		if((mWidgetUnderMouseCursor != NULL) && (mWidgetUnderMouseCursor->getGrabbed()))
		{
			// Check resizing first
			if((mResizableBorder != BORDER_NONE) && mDownOnBorder)
			{
				mWidgetUnderMouseCursor->resize(mResizableBorder,mMouseCursor->getPosition());

				return (mWidgetUnderMouseCursor != NULL);
			}
			// Check dragging
			else if(mWidgetUnderMouseCursor->getDragable())
			{
				// Dragging, which uses move function, works with pixel values (uninfluenced by parent dimensions!)
				mWidgetUnderMouseCursor->drag(xPixelOffset,yPixelOffset);

				return (mWidgetUnderMouseCursor != NULL);
			}
		}

		// Ignore disabled widgets
		if((w != NULL) && !(w->getEnabled()))
			return (mWidgetUnderMouseCursor != NULL);

		// Create MouseEventArgs, for use with any fired events
		MouseEventArgs args(w);
		args.position = mMouseCursor->getPosition();
		args.moveDelta.x = xPixelOffset;
		args.moveDelta.y = yPixelOffset;
		args.buttonMask = mButtonMask;
		args.keyModifiers = mKeyModifiers;

		// Create a boolean to track whether or not this injection caused any significant changes.
		bool changesMade = false;

		// The Widget underneath the mouse cursor has changed.
		if( mWidgetUnderMouseCursor != w )
		{
			if(mWidgetUnderMouseCursor != NULL)
			{
				changesMade |= mWidgetUnderMouseCursor->fireWidgetEvent(WIDGET_EVENT_MOUSE_LEAVE,args);
				// Restore cursor to default
				mMouseCursor->_setSkinType(mMouseCursor->mDefaultSkinType);
			}

			// Update pointer
			mWidgetUnderMouseCursor = w;

			if(mWidgetUnderMouseCursor != NULL)
			{
				changesMade |= mWidgetUnderMouseCursor->fireWidgetEvent(WIDGET_EVENT_MOUSE_ENTER,args);

				// Set the Timer to match this widgets hover time.
				mHoverTimer->setTimePeriod(mWidgetUnderMouseCursor->getHoverTime());
			}
		}

		// Notify the widget the cursor has moved.
		if(mWidgetUnderMouseCursor != NULL)
		{
			changesMade |= mWidgetUnderMouseCursor->fireWidgetEvent(WIDGET_EVENT_MOUSE_MOVE,args);

			// If Widget is grabable, change cursor
			if(mWidgetUnderMouseCursor->getDragable())
				mMouseCursor->_setSkinType("grabable");
			else
				mMouseCursor->_setSkinType(mMouseCursor->mDefaultSkinType);
		}

		// Check if cursor needs to change
		checkIfCursorOverResizableBorder(mMouseCursor->getPosition() - win->getPosition());

		return (mWidgetUnderMouseCursor != NULL);
	}

	bool GUIManager::injectMousePosition(const int& xPixelPosition, const int& yPixelPosition)
	{
		// do nothing if no active sheet is in use
		if(mActiveSheet == NULL)
			return false;

		// If the mouse is disabled, we do not accept this injection of input
		if( !mMouseCursor->getEnabled() ) 
			return false;

		Point oldPos = mMouseCursor->getPosition();
		// Update cursor's position as seen on the screen.
		mMouseCursor->setPosition(xPixelPosition,yPixelPosition);

		// Determine the offset and inject a mouse movement input.
		return injectMouseMove(xPixelPosition - oldPos.x,yPixelPosition - oldPos.y);
	}

	bool GUIManager::injectMouseWheelChange(float delta)
	{
		// do nothing if no active sheet is in use
		if(mActiveSheet == NULL)
			return false;

		// If the mouse is disabled, we do not accept this injection of input
		if( !mMouseCursor->getEnabled() ) 
			return false;

		MouseEventArgs args(mWidgetUnderMouseCursor);
		args.position = mMouseCursor->getPosition();
		args.wheelChange = delta;
		args.buttonMask = mButtonMask;
		args.keyModifiers = mKeyModifiers;

		if(mGUIManagerDesc.scrollLastClicked)
		{
			if(mLastClickedWidget != NULL)
			{
				Widget* w = mLastClickedWidget->getScrollableContainerWidget();
				if(w != NULL)
				{
					w->fireWidgetEvent(WIDGET_EVENT_MOUSE_WHEEL,args);
					return true;
				}
			}
		}
		else
		{
			if(mWidgetUnderMouseCursor != NULL)
			{
				Widget* w = mWidgetUnderMouseCursor->getScrollableContainerWidget();
				if(w != NULL)
				{
					w->fireWidgetEvent(WIDGET_EVENT_MOUSE_WHEEL,args);
					return (mWidgetUnderMouseCursor != NULL);
				}
			}
		}

		return (mWidgetUnderMouseCursor != NULL);
	}

	bool GUIManager::isSupportedCodePoint(Ogre::UTFString::unicode_char c)
	{
		return (mGUIManagerDesc.supportedCodePoints.find(c) != mGUIManagerDesc.supportedCodePoints.end());
	}

	void GUIManager::hoverTimerCallback()
	{
		if(mWidgetUnderMouseCursor != NULL)
		{
			WidgetEventArgs args(mWidgetUnderMouseCursor);
			mWidgetUnderMouseCursor->fireWidgetEvent(WIDGET_EVENT_ON_HOVER, args);
		}
	}

	void GUIManager::notifyViewportDimensionsChanged()
	{
		if(mGUIManagerDesc.viewport == NULL)
		{
			mViewportWidth = 0;
			mViewportHeight = 0;
		}
		else
		{
			mViewportWidth = mGUIManagerDesc.viewport->getActualWidth();
			mViewportHeight = mGUIManagerDesc.viewport->getActualHeight();
		}

		if(mActiveSheet != NULL)
			mActiveSheet->notifyViewport(&mViewportWidth,&mViewportHeight);
	}

	void GUIManager::renderQueueStarted(Ogre::uint8 id, const std::string& invocation, bool& skipThisQueue)
	{
	}

	void GUIManager::renderQueueEnded(Ogre::uint8 id, const std::string& invocation, bool& repeatThisQueue)
	{
		// Perform rendering of GUI
		if(mGUIManagerDesc.queueID == id)
		{
			draw();
		}
	}

	void GUIManager::setActiveSheet(Sheet* s)
	{
		if(mActiveSheet != NULL)
		{
			if(mWidgetUnderMouseCursor != NULL)
			{
				// Create MouseEventArgs, for use with any fired events
				MouseEventArgs args(mWidgetUnderMouseCursor);
				args.position = mMouseCursor->getPosition();
				args.buttonMask = mButtonMask;
				args.keyModifiers = mKeyModifiers;

				mWidgetUnderMouseCursor->fireWidgetEvent(WIDGET_EVENT_MOUSE_LEAVE,args);
			}
		}

		mActiveSheet = s;
		mWidgetUnderMouseCursor = mActiveSheet;
		mLastClickedWidget = NULL;

		if(mActiveSheet != NULL)
		{
			mActiveSheet->notifyViewport(&mViewportWidth,&mViewportHeight);

			mActiveSheet->_setGUIManager(this);

			// In the case where a sheet was unloaded, and later reloaded, make sure the Sheet is drawn correctly.
			// For example, prevent the scenario where a particular button was in the "Over" state when unloaded, but on
			// loading, should not be in the "Over" state because the cursor is not over the button.
			injectMouseMove(0,0);
		}
	}

	void GUIManager::setRenderQueueID(Ogre::uint8 id)
	{
		mGUIManagerDesc.queueID = id;
	}

	void GUIManager::setSceneManager(Ogre::SceneManager* sm)
	{
		// remove listener from previous scene manager
		if(mGUIManagerDesc.sceneManager != NULL)
			mGUIManagerDesc.sceneManager->removeRenderQueueListener(this);
		
		// update
		mGUIManagerDesc.sceneManager = sm;

		// add listener to new scene manager
		if(mGUIManagerDesc.sceneManager != NULL)
			mGUIManagerDesc.sceneManager->addRenderQueueListener(this);
	}

	void GUIManager::setScrollLastClicked(bool scroll)
	{
		mGUIManagerDesc.scrollLastClicked = scroll;
	}

	void GUIManager::setSupportedCodePoints(const std::set<Ogre::UTFString::code_point>& list)
	{
		mGUIManagerDesc.supportedCodePoints = list;
	}

	void GUIManager::setViewport(Ogre::Viewport* vp)
	{
		mGUIManagerDesc.viewport = vp;

		if(vp == NULL)
		{
			mViewportWidth = 0;
			mViewportHeight = 0;
		}
		else
		{
			mViewportWidth = vp->getActualWidth();
			mViewportHeight = vp->getActualHeight();
		}
	}
}
