Editor Themes
=============

This directory contains editor packages and themes that supports `blip` files.

Installing for Sublime Text
-----------------------------------

To install the syntax highlighting package for Sublime Text,
copy the directory `SublimeText/Blip` into the `Packages` folder, which can be opened in the menu with `Preferences > Browse Packages...`.

Installing for Notepad++
-----------------------------------

- Copy `Notepad++/BlipNotepadPlusPlus.xml` and `Notepad++/Techmantium_Girly.xml` into your Notepad++ themes directory, something like `C:\Program Files\Notepad++\themes\` or possibly `C:\Program Files (x86)\Notepad++\themes\`
- If the themes directory doesn't exist, simply create one in your `Notepad++` directory
- Launch Notepad++ and click `Settings` from the menu, then select `Style Configurator`
- You'll want to select a dark theme for maximum effect. `Techmantium_Girly` is provided free from [http://timtrott.co.uk/notepad-colour-schemes](http://timtrott.co.uk/notepad-colour-schemes).
- Close your theme window, and select menu item `Language > Define your language...` Click the `Import...` button
- Select `BliplayNotepadPlusPlus.xml` from your `Notepad++\themes` directory

Using in CodeMirror
-----------------------------------

Load the language script from `CodeMirror/mode/blip/blip.js` and use the mode `blip`:

```js
var editor = CodeMirror.fromTextArea(document.getElementById('code'), {
	mode: 'blip', // use mode 'blip'
	lineNumbers: true,
	matchBrackets: true,
});
```
