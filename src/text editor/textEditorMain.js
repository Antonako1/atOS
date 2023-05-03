// Import and define global variables
const readline = require('readline');
readline.emitKeypressEvents(process.stdin);
process.stdin.setRawMode(true);

let rowCount = 1;
textEditorMainFunc = function(){
    console.clear();
    readline.question("Start writing: ", file => {
        console.log(file);
        newInputCNSLE();
    })
    hotKeys()
    return "1"
}
checkAndAddRow = function(){
    // if press enter on
    // rowCount++
}
hotKeys = function(){
    const listener = process.stdin.on('keypress', (str, key) => {
        // Rotates piece 1 to right
        if (key.name === 'escape') {
            console.log("ok")
        }else if (key.name === "up"){
            lastMove = "rotate";
        } else if (key.name === 'left') {
            lastMove = "left";
        } else if (key.name === 'right') {
            lastMove = "right";
        }else if(key.name === "0"){
            process.exit(0)
        }
    });
}
textEditorMainFunc();
module.exports={
    textEditorMainFunc: textEditorMainFunc
}