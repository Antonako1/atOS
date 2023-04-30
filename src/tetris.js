// Import readline
const readline = require('readline');
// Define global variables
const width = 28;
const height = 18;
let board = [];
let blocksArr = [];
let intervalIds = [];
let block = "◻";
let lastMove;

// Defines spawnpoints
let blockY = 0;
let blockX = 14;
startGameTetris = function(){
    console.log("Starting game");
    // Set up the game board
    for(let i = 0; i < height; i++) {
        board[i] = Array(width).fill('◼');
    }

    // Draw test block
    spawnCube();

    // Call drawGame
    drawGame();

    // Get user-input
    readline.emitKeypressEvents(process.stdin);
    process.stdin.setRawMode(true);
    lastMove = "up";
    const listener = process.stdin.on('keypress', (str, key) => {
        // Rotates piece 1 to right
        if (key.name === 'up') {
            lastMove = "rotate";
            console.log(lastMove);
        } else if (key.name === 'left') {
            lastMove = "left";
            console.log(lastMove);
        } else if (key.name === 'right') {
            lastMove = "right";
            console.log(lastMove);
        }else if(key.name === "q"){
            intervalIds.forEach(id => clearInterval(id));
            // clear the array
            intervalIds = [];
            process.stdin.pause();
        }
    });
    let moving = setInterval(() => {
      moveSquare();
    }, 500);
    intervalIds.push(moving);
}

spawnCube = function(){
    board[blockY][blockX] = block;
}
// Function that moves the pieces
moveSquare = function(){
    switch(lastMove) {
        case "up":
            rotate();
            break;
        case "left":
            moveLEFT();
            break;
        case "right":
            moveRIGHT();
            break;
        }
        if(blockY + 1 == 18){
        }else{
            blockY += 1;
        }
        board[blockY][blockX] = block;
        moveAnimation();
        drawGame();
}
// 2 functions for moving and 1 for rotating
moveLEFT = function(){
    lastMove = "";
    if(blockX - 1 == -1){
    }else{
        blockX -= 1;
    }
    board[blockY][blockX] = block;
    moveAnimation();
    drawGame();
}
moveRIGHT = function(){
    lastMove = "";
    if(blockX + 1 == 28){
    }else{
        blockX += 1;
    }
    board[blockY][blockX] = block;
    moveAnimation();
    drawGame();
}
rotate = function(){
    lastMove = "";

}

drawGame = function(){
    console.clear();
    // Display the game board
    for(let i = 0; i < height; i++) {
        for(let j = 0; j < width; j++) {
            process.stdout.write(board[i][j]);
        }
        process.stdout.write('\n');
    }
}

moveAnimation = function(){
    // for(let i = 0; i < snakeArea.length - snakeLength; i++){
    //     if(snakeArea.length == 1) {
    //         continue;
    //     } else {
    //         snakeArea.shift();
    //     }

    //     for(let row = 0; row < board.length; row++){
    //         for(let col = 0; col < board[row].length; col++){
    //             let piece = board[row][col];
    //             if(piece === snake){
    //                 let hasSnake = false;
    //                 for(let k = 0; k < snakeArea.length; k++){
    //                     let snakePiece = snakeArea[k];
    //                     if(snakePiece[0] === row && snakePiece[1] === col){
    //                         hasSnake = true;
    //                         break;
    //                     }
    //                 }
    //                 if(!hasSnake){
    //                     board[row][col] = '◼';
    //                 }
    //             }
    //         }
    //     }
    // }
}



module.exports ={
    startGameTetris: startGameTetris
}