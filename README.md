# MonokumaHook 
A debug menu hook for Danganronpa: Trigger Happy Havoc.

A demo video can be found [here](https://youtu.be/vbG7M5s6R9w).
![](https://cdn.platea.moe/bread/Disloyal-Specific-Ynambu/EFOIZC6gkJ.png)

# Why?
I wanted to figure out what these debug options do, so I did.

Due to compiler optimization and what I assume to be debug preprocessor macros, the debug menu has been replaced with a single function that does nothing. (A single jump to a ret. No idea. See address `435B0` in your local disassembler.)<br>

Because of this, the EXE needs to be patched and hooked during runtime for the debug menu to appear properly.<br>

As a side effect of this, the debug menu might have missing data, crash the game, or do nothing at times.<sup>Here be dragons!</sup>

I do my best to try and restore most of the functionality as best i can figure out.

If you do find a dragon though, you can [create an issue](https://github.com/Visual-Novel-Decompilation-Project/monokuma-hook/issues) about it, and i'll take a look.

If you've figured out how to fight a dragon, you can [create a pull request](https://github.com/Visual-Novel-Decompilation-Project/monokuma-hook/pulls) instead.

# Prerequisites
- A controller (optional).
  - Yes, a controller. Anything steam recognizes is fine, as long as it works with the game.
    - Because some debug menu items only accept analog input.

# Controls
`F5` opens the debug menu.<br>
`F6` opens the player camera control menu.<br>
`F9` forcibly opens the debug menu (for testing purposes).<br>
`F10` opens the utilities and settings window.<br>

If using a keyboard, `Q` decreases values, `E` increases values. Also makes you extremely dizzy in some cases.

#### Keyboard Control Layout
`Z` = Cross<br>
`C` = Circle<br>
`X` = Square<br>
`V` = Triangle<br>

# Instructions 
1. Download the Hook.<br>
2. Inject the DLL<br>

#### DLL Injectors you can use:
<details>

<summary> Xenos </summary>
<a href="https://github.com/DarthTon/Xenos">https://github.com/DarthTon/Xenos</a>
<summary>Download Xenos from github.</summary>
<img src="https://cdn.platea.moe/public/msedge_Z1a5UBYfRU.png"/>
<li>Ask Windows Defender nicely not to delete it.</li>
<li>Start the game.</li>
<li>Run Xenos.exe as an administrator.</li>
<summary>You will now see the following:</summary>
<img src="https://cdn.platea.moe/public/Code_scgg9PxLEc.png"/>
<summary>Select the process "DR1_us.exe" as shown:</summary>
<img src="https://cdn.platea.moe/public/DR1_us_skNFv25jXJ.png"/>
<summary>Now add the DLL as shown:</summary>
<img src="https://cdn.platea.moe/public/Xenos_jw5nWH8dg0.png"/>
<summary>Finally, Xenos should look like this:</summary>
<img src="https://cdn.platea.moe/public/OY943BAm4e.png"/>
<summary>Press inject, and we're done!</summary>
<img src="https://cdn.platea.moe/public/DR1_us_KbP1mzqNB8.png"/>
<li>You can now close Xenos at this point.</li>
<br>
</details>

<details>
<summary>Cheat Engine</summary>
<li>Start the game.</li>
<li>Start Cheat Engine.</li>
<li>Select DR1_us.exe from the processes list.</li>
<img src="https://cdn.platea.moe/bread/Valuable-Old-Thunderbird/cheatengine-x86_64-SSE4-AVX2_e4OnB4haW1.jpg">
<li>Click on Memory View.</li>
<img src="https://cdn.platea.moe/bread/Zany-Sudden-Noddy/cheatengine-x86_64-SSE4-AVX2_0KzC89Zgqn.jpg">
<li>Select Tools -> Inject DLL</li>
<img src="https://cdn.platea.moe/bread/Mushy-Exotic-Spadefoot/cheatengine-x86_64-SSE4-AVX2_YmXoHhCAIV.jpg">
<li>Select MonokumaHook.dll</li>
<img src="https://cdn.platea.moe/bread/Winding-Windy-Gilamonster/cheatengine-x86_64-SSE4-AVX2_s55PVVXG3W.jpg">
<li>Select No to "Do you want to execute a function of the dll?"</li>
<img src="https://cdn.platea.moe/bread/Teal-Busy-Conure/cheatengine-x86_64-SSE4-AVX2_yxWZcxl9jv.jpg">
<li>Congratulations! You can now close Cheat Engine.</li>
<img src="https://cdn.platea.moe/bread/Worn-Real-Hoverfly/cheatengine-x86_64-SSE4-AVX2_dOkFzcCPgK.jpg">
</details>

- You can also bring your own DLL Injector of preference, if that's more of your thing.

3. Profit!


# Building
I use vcpkg for my dependencies. Your setup may vary, but the requirements are:

- [Detours](https://github.com/microsoft/Detours)
- [Kiero](https://github.com/Rebzzel/kiero) [Included as a submodule.]

Go to Kiero.h and change `KIERO_INCLUDE_D3D9EX` to `1`, otherwise the hook will fail.<br>
You may need to modify `CMakeLists.txt` to accomodate for your own setup for Detours, if you are not using vcpkg.

# License
This project is licensed under the [Apache License 2.0](https://github.com/Visual-Novel-Decompilation-Project/monokuma-hook/blob/main/LICENSE).
