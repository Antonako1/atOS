const moduleSNAKE = require("./snake.js");
const startGame = moduleSNAKE.startGame;
runSnake = function(){
    startGame();
}

module.exports = {
    runSnake: runSnake
};