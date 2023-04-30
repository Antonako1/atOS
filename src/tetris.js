// Import readline
const { spawn } = require('child_process');
const readline = require('readline');
const { pipeline } = require('stream');
// Define global variables
const width = 24;
const height = 18;
let board = [];
// All pieces
let blocksArr = [];
// Current active piece
let blocksArrActive = [];
let intervalIds = [];
let tetris = "◻";
let lastMove;
let rotationCounter = 0;

// Defines spawnpoints
let blockY = 0;
let blockX = 11;

let iPiece = false;
let oPiece = false;
let tPiece = false;
let jPiece = false;
let lPiece = false;
let sPiece = false;
let zPiece = false;

reset = function(){
    iPiece = false;
    oPiece = false;
    tPiece = false;
    jPiece = false;
    lPiece = false;
    sPiece = false;
    zPiece = false;
}

startGameTetris = function(){
    console.log("Starting game");
    // Set up the game board
    for(let i = 0; i < height; i++) {
        board[i] = Array(width).fill('◼');
    }
    // Spawn first block
    spawnCube();

    // Call drawGame
    drawGame();

    // Get user-input
    readline.emitKeypressEvents(process.stdin);
    process.stdin.setRawMode(true);
    const listener = process.stdin.on('keypress', (str, key) => {
        // Rotates piece 1 to right
        if (key.name === 'down') {
            lastMove = "rotate";
        } else if (key.name === 'left') {
            lastMove = "left";
        } else if (key.name === 'right') {
            lastMove = "right";
        }else if(key.name === "q"){
            intervalIds.forEach(id => clearInterval(id));
            // clear the array
            intervalIds = [];
            process.stdin.pause();
        }
    });
    let moving = setInterval(() => {
      moveSquare();
    }, 400);
    intervalIds.push(moving);
}

spawnCube = function(){
    // Randomly decide piece and spawn it
    let spawnNumber = Math.floor(Math.random() * 7) + 1;
    // if(spawnNumber === 1){
        straightDomino();
    // }else if(spawnNumber === 2){
        // squareDomino();
    // }else if(spawnNumber === 3){
        // tDomino();
    // }else if(spawnNumber === 4){
        // jDomino();
    // }else if(spawnNumber === 5){
        // lDomino();
    // }else if(spawnNumber === 6){
        // sDomino();
    // }else if(spawnNumber === 7){
        // zDomino();
    // }
}
// All the different pieces
straightDomino = function(){
    iPiece = true;
    board[blockY][blockX-2] = tetris
    blocksArrActive.push([blockY, blockX-2])

    board[blockY][blockX-1] = tetris
    blocksArrActive.push([blockY, blockX-1])

    board[blockY][blockX] = tetris
    blocksArrActive.push([blockY, blockX])

    board[blockY][blockX+1] = tetris
    blocksArrActive.push([blockY, blockX+1])
}
squareDomino = function(){
    oPiece = true;
    board[blockY][blockX-1] = tetris
    blocksArrActive.push([blockY, blockX-1])

    board[blockY][blockX] = tetris
    blocksArrActive.push([blockY, blockX])

    board[blockY+1][blockX-1] = tetris
    blocksArrActive.push([blockY+1, blockX-1])

    board[blockY+1][blockX] = tetris
    blocksArrActive.push([blockY+1, blockX])
}
tDomino = function(){
    tPiece = true;
    board[blockY][blockX-1] = tetris
    blocksArrActive.push([blockY, blockX-1])

    board[blockY][blockX] = tetris
    blocksArrActive.push([blockY, blockX])

    board[blockY][blockX+1] = tetris
    blocksArrActive.push([blockY, blockX+1])

    board[blockY+1][blockX] = tetris
    blocksArrActive.push([blockY+1, blockX])
}
jDomino = function(){
    jPiece = true;
    board[blockY][blockX] = tetris
    blocksArrActive.push([blockY, blockX])

    board[blockY+1][blockX] = tetris
    blocksArrActive.push([blockY+1, blockX])

    board[blockY+2][blockX] = tetris
    blocksArrActive.push([blockY+2, blockX])

    board[blockY+2][blockX-1] = tetris
    blocksArrActive.push([blockY+2, blockX-1])
}
lDomino = function(){
    lPiece = true;
    board[blockY][blockX] = tetris
    blocksArrActive.push([blockY, blockX])

    board[blockY+1][blockX] = tetris
    blocksArrActive.push([blockY+1, blockX])

    board[blockY+2][blockX] = tetris
    blocksArrActive.push([blockY+2, blockX])

    board[blockY+2][blockX+1] = tetris
    blocksArrActive.push([blockY+2, blockX+1])
}
sDomino = function(){
    sPiece = true;
    board[blockY][blockX+1] = tetris
    blocksArrActive.push([blockY, blockX+1])

    board[blockY][blockX] = tetris
    blocksArrActive.push([blockY, blockX])

    board[blockY+1][blockX] = tetris
    blocksArrActive.push([blockY+1, blockX])

    board[blockY+1][blockX-1] = tetris
    blocksArrActive.push([blockY+1, blockX-1])
}
zDomino = function(){
    zPiece = true;
    board[blockY][blockX-1] = tetris
    blocksArrActive.push([blockY, blockX-1])

    board[blockY][blockX] = tetris
    blocksArrActive.push([blockY, blockX])

    board[blockY+1][blockX] = tetris
    blocksArrActive.push([blockY+1, blockX])

    board[blockY+1][blockX+1] = tetris
    blocksArrActive.push([blockY+1, blockX+1])
}
// Function that moves the pieces
moveSquare = function(){
    // get current piece before moving it
    switch(lastMove) {
        case "rotate":
            rotate();
            break;
        case "left":
            moveLEFT();
            break;
        case "right":
            moveRIGHT();
            break;
    }
    
    // move the tetris piece down
    if(blockY + 1 == 18){
        
    } else {
        // move the piece down one row
        blockY += 1;
        for(let i = 0; i < blocksArrActive.length; i++){
            blocksArrActive[i][0] += 1;
        }
        moveAnimation();
        drawGame();
    }
}
checkCurrentPiece = function(par){
    if (iPiece == true) {
    } else if(oPiece == true) {
    } else if(tPiece == true) {
        // tDomino();

    } else if(jPiece == true) {
        // jDomino();

    } else if(lPiece == true) {
        // lDomino();        

    } else if(sPiece == true) {
        // sDomino();

    } else if(zPiece == true) {
        // zDomino();

    }
}
// 2 functions for moving and 1 for rotating
moveLEFT = function(){
    lastMove = "";
    if(blockX - 1 == -1){}else{
        // Update positions
        for(let i = 0; i < blocksArrActive.length; i++){
            blocksArrActive[i][1] -= 1;
        }
    }
    moveAnimation();
    drawGame();
}
moveRIGHT = function(){
    lastMove = "";
    if(blockX + 1 == 28){}else{
        // Update positions
        for(let i = 0; i < blocksArrActive.length; i++){
            blocksArrActive[i][1] += 1;
        }
    }
    moveAnimation();
    drawGame();
}

rotate = function(){
    lastMove = "";
    // Reset rotations
    if(rotationCounter === 4){
        rotationCounter = 0;
    }
    // Rotation for I-beam
    if (iPiece == true) {
        if(rotationCounter === 1 || rotationCounter === 3){
            // X-axis
            blocksArrActive[0][1] -=1;
            blocksArrActive[1][1];
            blocksArrActive[2][1] += 1;
            blocksArrActive[3][1] +=2;
            // Y-axis
            blocksArrActive[0][0];
            blocksArrActive[1][0] -= 1;
            blocksArrActive[2][0] -= 2;
            blocksArrActive[3][0] -=3;
        }else{
             // X-axis
             blocksArrActive[0][1] = blocksArrActive[2][1]
             blocksArrActive[1][1] = blocksArrActive[2][1]
             blocksArrActive[2][1] = blocksArrActive[2][1]
             blocksArrActive[3][1] = blocksArrActive[2][1]
             // Y-axis
             blocksArrActive[0][0] = blocksArrActive[0][0];
             blocksArrActive[1][0] += 1;
             blocksArrActive[2][0] += 2;
             blocksArrActive[3][0] += 3;
        }
        rotationCounter++
    } else if(oPiece == true) {
    } else if(tPiece == true) {
    } else if(jPiece == true) {
    } else if(lPiece == true) {
    } else if(sPiece == true) {
    } else if(zPiece == true) {
    }
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
    // First, clear the board of all tetris pieces
    for(let i = 0; i < height; i++) {
        for(let j = 0; j < width; j++) {
          if(board[i][j] === tetris) {
            board[i][j] = '◼';
          }
        }
      }
      
      // Then, draw the new position of the tetris pieces
      for(let i = 0; i < blocksArrActive.length; i++){
        let row = blocksArrActive[i][0];
        let col = blocksArrActive[i][1];
        if(row == 17 || checkHitbox() == true){
            // the piece has reached the bottom, spawn a new piece
            for(let k = 0; k < blocksArrActive.length; k++){
                blocksArr.push(blocksArrActive[k])
            }            
            blocksArrActive = [];
            blockX = 12;
            blockY = 0;
            reset();
            spawnCube();
        }else{
            board[row][col] = tetris;
        }
        for(let i = 0; i < blocksArr.length; i++){
          let row = blocksArr[i][0];
          let col = blocksArr[i][1];
          if(row == 18){}else{
              board[row][col] = tetris;
          }
        }
      }

}

// If active piece "dead" peace
checkHitbox = function(){
    for(let i = 0; i < blocksArr.length; i++){
      let row = blocksArr[i][0];
      let col = blocksArr[i][1];
      for(let j = 0; j < blocksArrActive.length; j++){
        let rowActive = blocksArrActive[j][0];
        let colActive = blocksArrActive[j][1];
        if(row - 1 === rowActive && col  === colActive){
          return true;
        }
      }
    }
    return false;
  }
  
checkForTetris = function(){

}
  





module.exports ={
    startGameTetris: startGameTetris
}