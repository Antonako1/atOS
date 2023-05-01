const moduleTETRIS = require("./tetris.js");
const startGameTetris = moduleTETRIS.startGameTetris;
runTetris = function(){
    startGameTetris();
}
module.exports = {
    runTetris: runTetris
};