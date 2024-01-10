# RLArenaCollisionDumper
 Standalone Windows program to dump all of Rocket League's arena collision meshes

## Usage
 - Download the prebuilt executable from the latest release (or build it yourself)
 - Start Rocket League, go into free play, then run the RLArenaCollisionDumper executable
 - All arena collision geometry will be written to `./collision-meshes/`

## How it works
 - Locates Rocket League's running process
 - Uses instruction pattern-scanning to find a function that has a necessary BulletPhysics world pointer as an argument
 - Allocates 8 bytes of memory inside Rocket League
 - Patches machine code into that BulletPhysics function to `mov [that memory we allocated], RCX` (`RCX` being the BulletPhysics world pointer argument)
 - Once the function is called, it grabs the BulletPhysics world pointer from the allocated memory
 - Follows the memory structures of the BulletPhysics library to find the geometry data (triangle index sets and vertices)
 - Saves all of that data to multiple files (one for each mesh)

## Future update compatibility
 The use of pattern scanning to locate the required function should mean this program won't need to change for future Rocket League updates unless something really drastic happens (i.e. move to UE5)

## Requirements
 - Rocket League running on Windows
