The Console extension ASI adds new console commands to Mass Effect 1/2/3 Legendary Edition

Commands added by this ASI:
* `savecam`: saves the position, yaw, and pitch of the camera to one of 100 slots. (Roll and FOV are not saved!) 
Must be followed by an integer, 0-99. Example usage: `savecam 0`
* `loadcam`: same as `savecam`, but sets the camera to a saved position. Should be used while in flycam/photo mode. (If in photo mode, the camera may be snapped back to a position within the restricted area. I suggest using a mod that removes photo mode restrictions for best effect). Saved positions persist between sessions in a file within the game install: "Mass Effect Legendary Edition\Game\ME{1|2|3}\Binaries\Win64\savedCams" 
* `remoteevent` or `re`: Triggers any kismet remote events with the provided name, just like `consoleevent` or `ce` triggers kismet console events. Example usage: `re my_event`

LE3 Only
* `streamlevelin`, `streamlevelout`, `onlyloadlevel`: Alters the streaming state of a level, just like the built-in commands of the same name in LE1 and LE2. Example usage: `streamlevelin BioA_CerMir`
