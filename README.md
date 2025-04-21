# A version of the A2 Engine created by Kiya Zadeh

## Building instructions

### Windows

Open the .sln using visual studio and build with the release settings. After building, make sure that the executable has the 4 SDL DLL files present

### OSX

Open the xcode project with xcode and check that the SDL frameworks are present in the build. Use Xcode's build for profiling to compile with release settings

### Linux

Use your distributions package manager to download the SDL, SDL_TTF, SDL_mixer, SDL_image dependencies

Use make lua and make box2d to build the dependencies, 

Afterwards, run make to build the engine in release mode, or make game_engine_linux_debug to build in debig mode

## Additional Docs on top of the standard A2 engine API

### Saving.SaveState(filename : string)

**param**: **filename** The name of the file to save to

Saves everything in the current scene, including all information about all actors, components, and global variables

### Saving.SaveScene(filename : string, previewData : LuaTable)

**param**: **filename** The name of the file to save to
**param**: **previewData** A Lua Table containing any data to be made available for preview upon calling Saving.GetAllSaves. Useful for loading screens

Saves all actors marked for saving using Scene.SetActorSaving to the specified file

### Scene.SetActorSaving(actor : Actor, save : bool)

**param**: **actor** A reference to an actor
**param**: **save** Should the actor be saved upon a call to Saving.SaveScene?

### Saving.LoadSaveFromFile(savefile : string, scene : string)

**param**: **savefile** The name of the save file to load
**param**: **scene** The name of the scene to use as a base

Loads the save by first loading the specified scene from the file as if by Scene.Load, then overwrites any actors sharing the same identity with the versions from the save file

### Saving.LoadSaveCurrentScene(savefile : string)

**param**: **savefile** The name of the save file to load

Loads the save by overwriting any actors in the current scene sharing the same identity with the version from the save file. All other actors are left unaffected and no scene transition occurs

### Saving.LoadState(savefile : string)

**param**: **savefile** The name of the save file to load

Loads the save from the savefile. This loads the save as a save state, so everything is exactly as it was at the time of saving, including globals and RNG seeding/advancement

### Saving.GetAllSaves()

**return**: A lua table where the keys are the accessible save files located in resources/saves and the values are lua tables containing the preview data

Gets all save files currently accessible along with any provided preview data

