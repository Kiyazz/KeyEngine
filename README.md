# A version of the A2 Engine created by Kiya Zadeh

Additional Docs on top of the standard A2 engine API

# Saving.SaveState(filename : string)

**param**: **filename** The name of the file to save to

Saves everything in the current scene, including all information about all actors, components, and global variables

# Saving.SaveScene(filename : string)

**param**: **filename** The name of the file to save to

Saves all actors marked for saving using Scene.SetActorSaving to the specified file

# Scene.SetActorSaving(actor : Actor, save : bool)

**param**: **actor** A reference to an actor
**param**: **save** Should the actor be saved upon a call to Saving.SaveScene?

