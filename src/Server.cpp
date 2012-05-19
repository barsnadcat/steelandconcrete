#include <pch.h>

#include <ServerGame.h>
#include <tclap/CmdLine.h>
#include <gflags/gflags.h>

#include <ReleaseVersion.h>
#include <ProtocolVersion.h>

DEFINE_bool(short_version, false, "Short version to use in build scripts");

void RunServer(int argc, char **argv)
{
	google::ParseCommandLineFlags(&argc, &argv, true);

	// Define the command line object.
	Ogre::String version = Ogre::StringConverter::toString(PROTOCOL_VERSION) +
		"." + Ogre::StringConverter::toString(RELEASE_VERSION);
	TCLAP::CmdLine cmd("Steelandconcrete server", ' ', version.c_str());

	TCLAP::ValueArg<Ogre::String> address("a", "address", "Server address", false, "localhost", "string");
	cmd.add(address);

	TCLAP::ValueArg<int32> port("p", "port", "Port", false, 4512, "int");
	cmd.add(port);

	TCLAP::ValueArg<int32> size("s", "size",
		"Map size: 1 - 162, 2 - 642, 3 - 2562, 4 - 10242, 5 - 40962, 6 - 163842, 7 - 655362 tiles",
		false, 4, "int");
	cmd.add(size);

	TCLAP::ValueArg<int32> seaLevel("l", "sealevel",
		"Sea level: 0-10000",
		false, 5000, "int");
	cmd.add(seaLevel);

	//TCLAP::SwitchArg shortVersion("v","short_version","Short version to use in build scripts", cmd, false);

	// Parse the args.
	cmd.parse( argc, argv );

	if (FLAGS_short_version)
    {
        std::cout << version;
        return;
    }

	ServerGame app(size.getValue());
	app.MainLoop(address.getValue(), port.getValue());
}
