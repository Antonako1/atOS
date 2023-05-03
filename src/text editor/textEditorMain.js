// Import and define global variables
const readline = require('readline');
readline.emitKeypressEvents(process.stdin);
process.stdin.setRawMode(true);

let writeMode = false;
let rowCount = 1;
chooseFileToEdit = function(){
    // absolute path or file.json path
    return fileToReturn;
}
textEditorMainFunc = function(){
    console.clear();
    hotKeys()
    console.log(writeMode);
    return "1"
}
checkAndAddRow = function(){
    // if press enter on
    // rowCount++
}
hotKeys = function(){
    const listener = process.stdin.on('keypress', (str, key) => {
        // Rotates piece 1 to right
        if (key.name === 'escape' && writeMode === true) {
            writemode = false;
            console.log('writemode::: ', writemode);
        }else if(key.name === "0"){
            listener._destroy();
        }else if(key.name === "i" && writeMode === false){
            writeMode = true;
            console.log('writeMode::: ', writeMode);
        }
    });
}
textEditorMainFunc();
module.exports={
    textEditorMainFunc: textEditorMainFunc
}