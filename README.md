# Quick Bottles

## Summary

Majora's Mask binds it's L button to toggling the mini-map. And let's be honest... you've almost never pressed this button on purpose.
The only time you DO press it on purpose, it's because you previously pressed it by accident. Maybe this functionality made sense on the original
N64 controller, where the entire left side was largely vestigial. But today, it may as well be a perfectly good controller button going
unused.

With this mod, the minimap is now toggled on or off in the mod config menu, freeing the L button to be mapped for a far better use: easy
access to your bottles. Gone are the days of having to pause and equip a bottle to catch a bug, drink a potion, or water a magic bean, only
to immediately unequip it moments later in favor of whatever you originally had in that slot. With this mod, you can skip the menus and access
any bottled item within moments.

Here's how it works: Quick Bottles draws a Bottle HUD in the bottom left corner, showing all of your bottles (unless you're using the 'Single' layout
setting, which only shows the currently selected bottle) and which bottle is currently selected. Tap L to use your selected bottle. Hold L and use
the DPad to change your selection. How the DPad directions cycle through your bottles will change to match your selected layout.

This mod IS compatible with the built-in DPad mod, as well as [Arrow Tweaks](https://thunderstore.io/c/zelda-64-recompiled/p/LoadingError404/Arrow_Tweaks/)
and potentially any other DPad mod that uses the original built-in mod as a base. *Note that maintaining this compatibility necessitated that when Link
uses a bottle with L, he automatically puts it away afterwards. This is intended behavior and will not be changed. HOWEVER, bottle behavior on the C Buttons works as normal, meaning that you can still use any glitches that depend on it.*

This is a great companion to mods that increase the difficulty, as potions and other healing items are now only a button press away!

In the future, the code that controls the minimap will be separated into it's own mod for use with other L-Button replacement mods.

This mod depends on ProxyMM's [KV Store](https://thunderstore.io/c/zelda-64-recompiled/p/ProxyMM/KV/) library to handle saving of Bottle HUD info
(Big shoutout to him for creating the mod, and accepting my PRs to add save slot functionality).

## HUD Layouts

The Bottle HUD has several layouts and presentation options to choose from. To demonstrate, here is the current contents of my bottles:

![Header](https://github.com/LT-Schmiddy/zelda64recomp-quick-bottles-mod/blob/custom-main/page_assets/bottles_in_inventory.png?raw=true)

The third bottle, the one with the Fairy, is the one selected in all images below:

### Single

![Layout Example](https://github.com/LT-Schmiddy/zelda64recomp-quick-bottles-mod/blob/custom-main/page_assets/single_layout.png?raw=true)

(An 'Auto-Hide' mode allows you to use allows you to use single-display normally, and your chosen layout option when selecting a bottle.
Special thanks to [Keanine](https://thunderstore.io/c/zelda-64-recompiled/p/Keanine/) for contributing this feature.)

### Horizontal

![Layout Example](https://github.com/LT-Schmiddy/zelda64recomp-quick-bottles-mod/blob/custom-main/page_assets/horizontal_layout.png?raw=true)

### Vertical - Bottom Up

![Layout Example](https://github.com/LT-Schmiddy/zelda64recomp-quick-bottles-mod/blob/custom-main/page_assets/bottom_up.png?raw=true)

### Vertical - Top Down

![Layout Example](https://github.com/LT-Schmiddy/zelda64recomp-quick-bottles-mod/blob/custom-main/page_assets/top_down.png?raw=true)

### Two Rows

![Layout Example](https://github.com/LT-Schmiddy/zelda64recomp-quick-bottles-mod/blob/custom-main/page_assets/two_rows.png?raw=true)

### Two Columns

![Layout Example](https://github.com/LT-Schmiddy/zelda64recomp-quick-bottles-mod/blob/custom-main/page_assets/two_columns.png?raw=true)

## HUD Options

All layout options support a Round Robin mode, where the selected bottle will always be displayed in the first position.

### Round Robin - Horizontal

![Layout Example](https://github.com/LT-Schmiddy/zelda64recomp-quick-bottles-mod/blob/custom-main/page_assets/rr_horizontal.png?raw=true)

### Round Robin - Vertical

![Layout Example](https://github.com/LT-Schmiddy/zelda64recomp-quick-bottles-mod/blob/custom-main/page_assets/rr_vertical.png?raw=true)

### Round Robin - Two Rows

![Layout Example](https://github.com/LT-Schmiddy/zelda64recomp-quick-bottles-mod/blob/custom-main/page_assets/rr_two_rows.png?raw=true)

Normally, the space to draw a bottle is reserved by the Bottle HUD, even if that bottle hasn't been acquired or was otherwise lost.
While this can be a good indicator of which bottles are missing normally, this display style may not be to everyone's taste, and can
look particularly bad in Round Robin mode. Therefore, the Horizontal layout and both Vertical layouts also have an alternative,
'Gapless' variant which sequentially displays only the bottles currently in your inventory.

### Missing a Bottle - Horizontal

![Layout Example](https://github.com/LT-Schmiddy/zelda64recomp-quick-bottles-mod/blob/custom-main/page_assets/missing_bottle_horizontal.png?raw=true)

### Missing a Bottle - Round Robin - Horizontal

![Layout Example](https://github.com/LT-Schmiddy/zelda64recomp-quick-bottles-mod/blob/custom-main/page_assets/missing_bottle_rr_horizontal.png?raw=true)

### Missing a Bottle - Round Robin - Horizontal Gapless

![Layout Example](https://github.com/LT-Schmiddy/zelda64recomp-quick-bottles-mod/blob/custom-main/page_assets/missing_bottle_rr_horizontal_gapless.png?raw=true)

If you don't like the transparency used to indicate your selected bottle, a 'Bordered' mode is also available.

![Layout Example](https://github.com/LT-Schmiddy/zelda64recomp-quick-bottles-mod/blob/custom-main/page_assets/horizontal_border_selection.png?raw=true)

Please report any issues you come across on GitHub!


## Writing mods

See [this document](https://hackmd.io/fMDiGEJ9TBSjomuZZOgzNg) for an explanation of the modding framework, including how to write function patches and perform interop between different mods.

## Tools

You'll need to install `clang` and `make` to build this template.

* On Windows, using [chocolatey](https://chocolatey.org/) to install both is recommended. The packages are `llvm` and `make` respectively.
  * The LLVM 19.1.0 [llvm-project](https://github.com/llvm/llvm-project) release binary, which is also what chocolatey provides, does not support MIPS correctly. The solution is to install 18.1.8 instead, which can be done in chocolatey by specifying `--version 18.1.8` or by downloading the 18.1.8 release directly.
* On Linux, these can both be installed using your distro's package manager.
* On MacOS, these can both be installed using Homebrew. Apple clang won't work, as you need a mips target for building the mod code.

On Linux and MacOS, you'll need to also ensure that you have the `zip` utility installed.

You'll also need to build [N64Recomp](https://github.com/N64Recomp/N64Recomp) for the `RecompModTool` utility.

## Building

* First, run `make` (with an optional job count) to build the mod code itself.
* Next, run the `RecompModTool` utility with `mod.toml` as the first argument and the build dir (`build` in the case of this template) as the second argument.
  * This will produce your mod's `.nrm` file in the build folder.
  * If you're on MacOS, you may need to specify the path to the `clang` and `ld.lld` binaries using the `CC` and `LD` environment variables, respectively.

## Updating the Majora's Mask Decompilation Submodule

Mods can also be made with newer versions of the Majora's Mask decompilation instead of the commit targeted by this repo's submodule.
To update the commit of the decompilation that you're targeting, follow these steps:

* Build the [N64Recomp](https://github.com/N64Recomp/N64Recomp) repo and copy the N64Recomp executable to the root of this repository.
* Build the version of the Majora's Mask decompilation that you want to update to and copy the resulting .elf file to the root of this repository.
* Update the `mm-decomp` submodule in your clone of this repo to point to the commit you built in the previous step.
* Run `N64Recomp generate_symbols.toml --dump-context`
* Rename `dump.toml` and `data_dump.toml` to `mm.us.rev1.syms.toml` and `mm.us.rev1.datasyms.toml` respectively.
  * Place both files in the `Zelda64RecompSyms` folder.
* Try building.
  * If it succeeds, you're done.
  * If it fails due to a missing header, create an empty header file in the `include/dummy_headers` folder, with the same path.
    * For example, if it complains that `assets/objects/object_cow/object_cow.h` is missing, create an empty `include/dummy_headers/objects/object_cow.h` file.
  * If RecompModTool fails due to a function "being marked as a patch but not existing in the original ROM", it's likely that function you're patching was renamed in the Majora's Mask decompilation.
    * Find the relevant function in the map file for the old decomp commit, then go to that address in the new map file, and update the reference to this function in your code with the new name.
