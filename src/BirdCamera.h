#ifndef BIRDCAMERA_H
#define BIRDCAMERA_H

#include <Ogre.h>
#include <OIS.h>
#include <OgreAL.h>

static const Ogre::Real zoomAcceleration = 0.00002;
static const Ogre::Degree rotationAcceleration(0.000002);

class BirdCamera
{
public:
    BirdCamera();
    void SetHorizontalSpeed(int aSpeed) { mHorizontalSpeed = aSpeed * rotationAcceleration; }
    void SetVerticalSpeed(int aSpeed) { mVerticalSpeed = aSpeed * rotationAcceleration; }
    void ZoomIn() { mZoomSpeed -= zoomAcceleration; }
    void ZoomOut() { mZoomSpeed += zoomAcceleration; }
    void UpdatePosition(unsigned long aTime);
    Ogre::Ray MouseToRay(const OIS::MouseState &aState) const;
    void Goto(const Ogre::Vector3 &aPosition);
    void SetDistance(Ogre::Real aDistance);
    ~BirdCamera();
protected:
private:
    Ogre::Radian mVerticalSpeed;
    Ogre::Radian mHorizontalSpeed;
    Ogre::Real mZoomSpeed;
    Ogre::SceneNode* mCameraHolder;
    Ogre::SceneNode* mCameraNode;
};

#endif // BIRDCAMERA_H
