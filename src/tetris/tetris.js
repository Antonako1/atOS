// Import readline
const readline = require('readline');
// Define global variables
const twidth = 18;
const theight = 22;
let boardTetris = [];
// All pieces
let blocksArr = [];
// Current active piece
let blocksArrActive = [];
let intervalIds = [];
let tetris = "[]";
let lastMove;
let points = 0;

// Defines spawnpoints
let blockY = 0;
let blockX = 10;

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
    // Set up the game boardTetris
    for(let i = 0; i < theight; i++) {
        boardTetris[i] = Array(twidth).fill('  ');
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
            lastMove = "faster"
        }else if (key.name === "up"){
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
    }, 300);
    intervalIds.push(moving);
}

spawnCube = function(){
    // Define array of different domino types
    const dominoTypes = [straightDomino, squareDomino, tDomino, jDomino, lDomino, sDomino, zDomino];
    
    // Shuffle the array using Fisher-Yates algorithm
    for (let i = dominoTypes.length - 1; i > 0; i--) {
        const j = Math.floor(Math.random() * (i + 1));
        [dominoTypes[i], dominoTypes[j]] = [dominoTypes[j], dominoTypes[i]];
    }
    
    // Spawn the first element in the shuffled array
    dominoTypes[0]();
}

// All the different pieces
straightDomino = function(){
    iPiece = true;
    boardTetris[blockY][blockX-2] = tetris
    blocksArrActive.push([blockY, blockX-2])

    boardTetris[blockY][blockX-1] = tetris
    blocksArrActive.push([blockY, blockX-1])

    boardTetris[blockY][blockX] = tetris
    blocksArrActive.push([blockY, blockX])

    boardTetris[blockY][blockX+1] = tetris
    blocksArrActive.push([blockY, blockX+1])
}
squareDomino = function(){
    oPiece = true;
    boardTetris[blockY][blockX-1] = tetris
    blocksArrActive.push([blockY, blockX-1])

    boardTetris[blockY][blockX] = tetris
    blocksArrActive.push([blockY, blockX])

    boardTetris[blockY+1][blockX-1] = tetris
    blocksArrActive.push([blockY+1, blockX-1])

    boardTetris[blockY+1][blockX] = tetris
    blocksArrActive.push([blockY+1, blockX])
}
tDomino = function(){
    tPiece = true;
    boardTetris[blockY][blockX-1] = tetris
    blocksArrActive.push([blockY, blockX-1])

    boardTetris[blockY][blockX] = tetris
    blocksArrActive.push([blockY, blockX])

    boardTetris[blockY+1][blockX] = tetris
    blocksArrActive.push([blockY+1, blockX])

    boardTetris[blockY][blockX+1] = tetris
    blocksArrActive.push([blockY, blockX+1])
}
jDomino = function(){
    jPiece = true;
    boardTetris[blockY+2][blockX-1] = tetris
    blocksArrActive.push([blockY+2, blockX-1])

    boardTetris[blockY][blockX] = tetris
    blocksArrActive.push([blockY, blockX])

    boardTetris[blockY+1][blockX] = tetris
    blocksArrActive.push([blockY+1, blockX])

    boardTetris[blockY+2][blockX] = tetris
    blocksArrActive.push([blockY+2, blockX])
}
lDomino = function(){
    lPiece = true;
    boardTetris[blockY][blockX] = tetris
    blocksArrActive.push([blockY, blockX])

    boardTetris[blockY+1][blockX] = tetris
    blocksArrActive.push([blockY+1, blockX])

    boardTetris[blockY+2][blockX] = tetris
    blocksArrActive.push([blockY+2, blockX])

    boardTetris[blockY+2][blockX+1] = tetris
    blocksArrActive.push([blockY+2, blockX+1])
}
sDomino = function(){
    sPiece = true;
    
    boardTetris[blockY][blockX] = tetris
    blocksArrActive.push([blockY, blockX])
    
    boardTetris[blockY+1][blockX] = tetris
    blocksArrActive.push([blockY+1, blockX])
    
    boardTetris[blockY+1][blockX-1] = tetris
    blocksArrActive.push([blockY+1, blockX-1])

    boardTetris[blockY][blockX+1] = tetris
    blocksArrActive.push([blockY, blockX+1])
}
zDomino = function(){
    zPiece = true;
    boardTetris[blockY][blockX-1] = tetris
    blocksArrActive.push([blockY, blockX-1])

    boardTetris[blockY][blockX] = tetris
    blocksArrActive.push([blockY, blockX])

    boardTetris[blockY+1][blockX] = tetris
    blocksArrActive.push([blockY+1, blockX])

    boardTetris[blockY+1][blockX+1] = tetris
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
            moveLeftTetris();
            break;
        case "right":
            moveRightTetris();
            break;
        case "faster":
            movePiece();
            lastMove = "";
            break;
    }
    movePiece();
}
movePiece = function(){
    // move the tetris piece down
    if(blockY + 1 == 22){
        
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
moveLeftTetris = function(){
    lastMove = "";
    if(blockX - 1 == -1){}else{
        // Update positions
        if(checkLegalLeft() == true && checkHitboxLeft() == true){
            blocksArrActive[0][1] -= 1;
            blocksArrActive[1][1] -= 1;
            blocksArrActive[2][1] -= 1;
            blocksArrActive[3][1] -= 1;
        }
        }
    moveAnimation();
    drawGame();
}
checkLegalLeft = function(){
    if(blockX - 1 == -1){}else{
        // Update positions
        for(let i = 0; i < blocksArrActive.length; i++){
            if(blocksArrActive[i][1] - 1 <= twidth-twidth-1){
                return false;
            }
        }
        return true
    }
}
checkHitboxLeft = function(){
    for(let i = 0; i < blocksArrActive.length; i++){
        let rowActive = blocksArrActive[i][0];
        let colActive = blocksArrActive[i][1] - 1;
        for(let j = 0; j < blocksArr.length; j++){
            let row = blocksArr[j][0];
            let col = blocksArr[j][1];
            if(row === rowActive && col === colActive){
                return false; // There is a collision
            }
        }
    }
    return true;
}
moveRightTetris = function(){
    lastMove = "";
    if(blockX + 1 == 28){}else{
        // Update positions
        if(checkLegalRight() == true && checkHitboxRight() == true){
            blocksArrActive[0][1] += 1;
            blocksArrActive[1][1] += 1;
            blocksArrActive[2][1] += 1;
            blocksArrActive[3][1] += 1;
        }
    }
    moveAnimation();
    drawGame();
}
checkLegalRight = function(){
    if(blockX - 1 == -1){}else{
        for(let i = blocksArrActive.length - 1; i >= 0; i--){
            if(blocksArrActive[i][1] + 1 >= twidth){
                return false;
            }
        }
        return true;
    }
}
checkHitboxRight = function(){
    for(let i = 0; i < blocksArrActive.length; i++){
        let rowActive = blocksArrActive[i][0];
        let colActive = blocksArrActive[i][1] + 1;
        for(let j = 0; j < blocksArr.length; j++){
            let row = blocksArr[j][0];
            let col = blocksArr[j][1];
            if(row === rowActive && col === colActive){
                return false; // There is a collision
            }
        }
    }
    return true;
}
rotate = function(){
    lastMove = "";
    const centerBlock = blocksArrActive[1];  
    // Check if rotating will move the tetris piece out of boundaries
    if(checkRotation(centerBlock) == true){
        // Rotate the tetris piece
        for (let i = 0; i < blocksArrActive.length; i++) {
            const relX = blocksArrActive[i][0] - centerBlock[0];
            const relY = blocksArrActive[i][1] - centerBlock[1];
            blocksArrActive[i][0] = centerBlock[0] - relY;
            blocksArrActive[i][1] = centerBlock[1] + relX;
        }
    }
}
checkRotation = function(centerBlock){
    for (let i = 0; i < blocksArrActive.length; i++) {
        const relX = blocksArrActive[i][0] - centerBlock[0];
        const relY = blocksArrActive[i][1] - centerBlock[1];
        const newX = centerBlock[0] - relY;
        const newY = centerBlock[1] + relX;
        if (newX < 0 || newX >= theight || newY < 0 || newY >= twidth) {
            return false; // Don't rotate if it will move out of boundaries
        }
    }
    return true;
}
drawGame = function(){
    console.clear();
    console.log();
    // Display the top border
    console.log('╔' + '═'.repeat(twidth * 2 - 1) + '╗');
    
    // Display the game boardTetris
    for(let i = 0; i < theight; i++) {
        process.stdout.write('║');
        for(let j = 0; j < twidth; j++) {
            process.stdout.write(boardTetris[i][j]);
        }
        process.stdout.write('║\n');
    }
    
    // Display the bottom border
    console.log('╚' + '═'.repeat(twidth * 2 - 1) + '╝');
    
    console.log("Move using arrow keys, Up arrow rotes, Down arrow makes piece move quicker");
    console.log("Points: " + points);
}



moveAnimation = function(){
    // First, clear the boardTetris of all tetris pieces
    for(let i = 0; i < theight; i++) {
        for(let j = 0; j < twidth; j++) {
          if(boardTetris[i][j] === tetris) {
              boardTetris[i][j] = '  ';
          }
        }
      }
      
      // Then, draw the new position of the tetris pieces
      for(let i = 0; i < blocksArrActive.length; i++){
        let row = blocksArrActive[i][0];
        let col = blocksArrActive[i][1];
        if(row == 21 || checkHitbox() == true){
            // the piece has reached the bottom, spawn a new piece
            for(let k = 0; k < blocksArrActive.length; k++){
                blocksArr.push(blocksArrActive[k])
            }            
            blocksArrActive = [];
            blockY = 0;
            blockX = 10;
            checkForTetris();
            reset();
            spawnCube();
        }else{
            if(row  == undefined || col == undefined){
                // Checks for undefined
            }else{
                boardTetris[row][col] = tetris;
            }
        }
        for(let i = 0; i < blocksArr.length; i++){
          let row = blocksArr[i][0];
          let col = blocksArr[i][1];
          if(row == 18){}else{
              boardTetris[row][col] = tetris;
          }
        }
      }

}
// If active piece "dead" peace
// ^^^????????????^^^
checkHitbox = function(){
    for(let i = 0; i < blocksArrActive.length; i++){
        // Take one y and x at a time
        let rowActive = blocksArrActive[i][0];
        let colActive = blocksArrActive[i][1];
        for(let j = 0; j < blocksArr.length; j++){
            let row = blocksArr[j][0] - 1;
            let col = blocksArr[j][1];
            // Compare for collisions
            if(row === rowActive && col === colActive){
                return true; // There is a collision
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