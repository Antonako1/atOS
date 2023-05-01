const readline = require('readline');

// Make these global
const width = 20;
const height = 10;
let board = [];
let snakeArea = [];
let apples = [];
let snake = "[]"; 
let apple = "()";
let snakeLength = 1;
let snakeX = 10;
let snakeY = 5;
let intervalIds1 = [];
let lastMoveSnake = null;
startGame = function(){
// ◻   
// Initialize snake

// Set up the game board
for(let i = 0; i < height; i++) {
    board[i] = Array(width).fill('  ');
}

// Define the initial position of the snake
board[snakeY][snakeX] = snake;
snakeArea.push([snakeY, snakeX])
displayBoard();


    readline.emitKeypressEvents(process.stdin);
    process.stdin.setRawMode(true);
    lastMoveSnake = "up";
    const listener = process.stdin.on('keypress', (str, key) => {
        if (key.name === 'up') {
          lastMoveSnake = "up";
        } else if (key.name === 'down') {
          
          lastMoveSnake = "down";
        } else if (key.name === 'left') {
          
          lastMoveSnake = "left";
        } else if (key.name === 'right') {
          lastMoveSnake = "right";
        }else if(key.name === "q"){
            process.stdin.pause();
        }
    });
    let moving = setInterval(() => {
      moveSquare();

    }, 300);
    intervalIds1.push(moving);
    let spawnFrequency = 3000; // Initial spawn frequency
    let intervalApple = setInterval(() => {
        spawnApples();
        
        // Decrease spawn frequency as snake grows longer
        if (snakeLength > 5) {
            spawnFrequency = 2000;
        }
        if (snakeLength > 10) {
            spawnFrequency = 1000;
        }
        if (snakeLength > 15) {
            spawnFrequency = 500;
        }
        if (snakeLength > 20) {
            spawnFrequency = 250;
        }
    }, spawnFrequency);
    intervalIds1.push(intervalApple);
      
    
}
moveSquare = function(){
    switch(lastMoveSnake) {
        case "up":
            moveUP();
            break;
        case "down":
            moveDOWN();
            break;
        case "left":
            moveLEFT();
            break;
        case "right":
            moveRIGHT();
            break;
    }
}

moveUP = function(){
    if(snakeY - 1 == -1){}else{
        snakeY -= 1;
        
    }
    board[snakeY][snakeX] = snake;
    if(checkDead(snakeY, snakeX) == true){}else{
        snakeArea.push([snakeY, snakeX])
    }
    checkLength();
    ateApple(snakeY, snakeX)
    displayBoard();
}
moveDOWN = function(){
    if(snakeY + 1 == 10){

    }else{
        snakeY += 1;
        
    }
    board[snakeY][snakeX] = snake;
    if(checkDead(snakeY, snakeX) == true){}else{
        snakeArea.push([snakeY, snakeX])
    }
    checkLength();
    ateApple(snakeY, snakeX)
    displayBoard();
   
}
moveLEFT = function(){
    if(snakeX - 1 == -1){

    }else{
        snakeX -= 1;
        
    }
    board[snakeY][snakeX] = snake;
    if(checkDead(snakeY, snakeX) == true){}else{
        snakeArea.push([snakeY, snakeX])
    }
    checkLength();
    ateApple(snakeY, snakeX)
    displayBoard();
    
}
moveRIGHT = function(){
    if(snakeX + 1 == 20){

    }else{
        snakeX += 1;
        
    }
    board[snakeY][snakeX] = snake;
    if(checkDead(snakeY, snakeX) == true){}else{
        snakeArea.push([snakeY, snakeX])
    }
    checkLength();
    ateApple(snakeY, snakeX)
    displayBoard();
    
    
}

displayBoard = function(){
    console.clear();
    console.log("Move using arrow keys");

    // Set interval to spawn apples

    // Display the game board
    for(let i = 0; i < height; i++) {
        for(let j = 0; j < width; j++) {
            if (i === 0 || i === height - 1) {
                process.stdout.write('═');
            } else if (j === 0 || j === width - 1) {
                process.stdout.write('║');
            } else {
                process.stdout.write(board[i][j]);
            }
        }
        process.stdout.write('\n');
    }
}

ateApple = function(y, x){
    let coords = [y, x];
    for (let i = 0; i < apples.length; i++) {
        if (apples[i][0] === coords[0] && apples[i][1] === coords[1]) {
            snakeLength++;
            apples.splice(i, 0)
            // board[apples[i][0]][apples[i][1]] = '  ';
            return true;
        }
    }
    return false;
} // ◻   
// Removes non-existent $snake pieces and replaces them with   
checkLength = function(){
    for(let i = 0; i < snakeArea.length - snakeLength; i++){
        if(snakeArea.length == 1) {
            continue;
        } else {
            snakeArea.shift();
        }

        for(let row = 0; row < board.length; row++){
            for(let col = 0; col < board[row].length; col++){
                let piece = board[row][col];
                if(piece === snake){
                    let hasSnake = false;
                    for(let k = 0; k < snakeArea.length; k++){
                        let snakePiece = snakeArea[k];
                        if(snakePiece[0] === row && snakePiece[1] === col){
                            hasSnake = true;
                            break;
                        }
                    }
                    if(!hasSnake){
                        board[row][col] = '  ';
                    }
                }
            }
        }
    }
}
spawnApples = function() {
    // Spawn apple coords
    let randomY = Math.floor(Math.random() * 10);
    let randomX = Math.floor(Math.random() * 20);
    let randomCombined = [randomY, randomX]

    // Check if apple is spawning on snake, if so, return
    for (let i = 0; i < snakeArea.length; i++) {
        if (snakeArea[i][0] === randomCombined[0] && snakeArea[i][1] === randomCombined[1]) {
            return;
        }
    }

    // Add apple to board and apples array
    board[randomY][randomX] = apple;
    apples.push([randomY, randomX])
}
checkDead = function(y, x){
    let coords = [y, x];
    for (let i = 0; i < snakeArea.length; i++) {
        if (snakeArea[i][0] === coords[0] && snakeArea[i][1] === coords[1]) {
            intervalIds1.forEach(id => clearInterval(id));
            // clear the array
            intervalIds1 = [];
            setTimeout(() => {
                console.log("You died with " + snakeLength + " apple(s) eaten");
                console.log(global.rootText + " ");
            }, 200);
            return true;
        }
    }
    return false;
}

module.exports = {
    startGame: startGame
};