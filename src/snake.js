const readline = require('readline');
// Make these global
const width = 20;
const height = 10;
let board = [];
let snakeArea = [];
let snake = "◻"; 
let snakeLength = 1;
let snakeX = 10;
let snakeY = 5;
startGame = function(){
// ◻ ◼
// Initialize snake

// Set up the game board
for(let i = 0; i < height; i++) {
    board[i] = Array(width).fill('◼');
}

// Define the initial position of the snake
board[snakeY][snakeX] = snake;
snakeArea.push([snakeY, snakeX])
displayBoard();



    readline.emitKeypressEvents(process.stdin);
    process.stdin.setRawMode(true);
    
    const listener = process.stdin.on('keypress', (str, key) => {
        if (key.name === 'up') {
          moveUP();
        } else if (key.name === 'down') {
          moveDOWN();
        } else if (key.name === 'left') {
          moveLEFT();
        } else if (key.name === 'right') {
          moveRIGHT();
        }
      });
      
      
      
    
}
moveUP = function(){
    if(snakeY - 1 == -1){}else{
        snakeY -= 1;
    }
    board[snakeY][snakeX] = snake;
    displayBoard();
    if(checkDead(snakeY, snakeX) == true){}else{
        snakeArea.push([snakeY, snakeX])
    }
}
moveDOWN = function(){
    if(snakeY + 1 == 10){

    }else{
        snakeY += 1;
    }
    board[snakeY][snakeX] = snake;
    displayBoard();
    if(checkDead(snakeY, snakeX) == true){}else{
        snakeArea.push([snakeY, snakeX])
    }
}
moveLEFT = function(){
    if(snakeX - 1 == -1){

    }else{
        snakeX -= 1;
    }
    board[snakeY][snakeX] = snake;
    displayBoard();
    if(checkDead(snakeY, snakeX) == true){}else{
        snakeArea.push([snakeY, snakeX])
    }
}
moveRIGHT = function(){
    if(snakeX + 1 == 20){

    }else{
        snakeX += 1;
    }
    board[snakeY][snakeX] = snake;
    displayBoard();
    if(checkDead(snakeY, snakeX) == true){}else{
        snakeArea.push([snakeY, snakeX])
    }
    
}

displayBoard = function(){
    console.clear()
    console.log(`$$$$$$\\                      $$\\                 
$$  __$$\\                     $$ |                
$$ /  \\__|$$$$$$\\   $$$$$$\\  $$ |  $$\\  $$$$$$\\  
\\$$$$$$\\  $$  __$$\\  \\____$$\\ $$ | $$  |$$  __$$\\ 
 \\____$$\\ $$ |  $$ | $$$$$$$ |$$$$$$  / $$$$$$$$ |
$$\\   $$ |$$ |  $$ |$$  __$$ |$$  _$$<  $$   ____|
\\$$$$$$  |$$ |  $$ |\\$$$$$$$ |$$ | \\$$\\ \\$$$$$$$\\ 
 \\______/ \\__|  \\__| \\_______|\\__|  \\__| \\_______|
                                                  
                                                  
                                                   `);
    console.log("Move using arrow keys");
    // Display the game board
    for(let i = 0; i < height; i++) {
        for(let j = 0; j < width; j++) {
            process.stdout.write(board[i][j]);
        }
        process.stdout.write('\n');
    }
}
ateApple = function(){
    console.log("ate");
    snakeLength++;
}
checkDead = function(y, x){
    let coords = [y, x];
    for (let i = 0; i < snakeArea.length; i++) {
        if (snakeArea[i][0] === coords[0] && snakeArea[i][1] === coords[1]) {
            console.log("You died with " + snakeLength + " apples eaten");
            // Call process.stdin.pause() to stop the listener
            process.stdin.pause();
            return true;
        }
    }
    return false;
}


  
startGame();
module.exports = {
    startGame: startGame
};