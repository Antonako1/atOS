/* 
atOS - at Operating Systems develompents
All rights reserved ©
*/




// todo fix "rmv" , save highscore games
// Imports consoleReadline and all others
const moduleCMDLETS = require("./src/cmdlets.js");
const readLineCNSLE = moduleCMDLETS.readLineCNSLE;

const moduleWRITEDATA = require("./src/writeData.js");
const writeData = moduleWRITEDATA.writeData; 

const moduleREADDATA= require("./src/readData.js");
const readData = moduleREADDATA.readData;

const moduleREMOVE= require("./src/remove.js");
const remove = moduleREMOVE.remove;

const moduleFind= require("./src/findItem.js");
const findItemMain = moduleFind.findItemMain;

const moduleDecrypt= require("./src/decryptData.js");
const decryptData = moduleDecrypt.decryptData;

const moduleTimer= require("./src/timer/timer.js");
const timerMain = moduleTimer.timerMain;
const checkTimer = moduleTimer.checkTimer;
let cmdboo=false;
let fileType, fileName, fileData, fileToRead, result;
// start
console.clear();
console.log("             d8     ,88~-_   ,d88~~\\ ");
console.log("  /~~~8e  _d88__  d888   \\  8888    ");
console.log("      88b  888   88888    | `Y88b   ");
console.log(" e88~-888  888   88888    |  `Y88b, ");
console.log("C888  888  888    Y888   /     8888 ");
console.log(" \"88_-888  \"88_/   `88_-~   \\__88P' ");
console.log("\n atOS is now running, press backspace to continue");
console.log("type help for list of commands");                            

/*
Normal version of root, if it gets deleted
{
  "name": "root",
  "path": "root/",
  "filetype": "folder",
  "filedata": []
}
*/

// Import node readfile module
const fs = require('fs');

global.data = fs.readFileSync('files/file.json', 'utf8');
global.data = JSON.parse(global.data);
// Start new readline process
const readline = require('readline').createInterface({
    input: process.stdin,
    output: process.stdout,
});

global.folder = global.root;
// New input after previous one was done
newInputCNSLE = function(){
  readline.question(global.rootText + " " , command => {
    cmdboo = false;
    // Identify commands
    // Timer question prompts
    if(command === "timer"){
      readline.question("Set timer for x minutes:  ",  time => {
        timerMain(time);
        cmdboo = true;
        newInputCNSLE();
      })
      return;
    }else if(command === "timer check"){
      checkTimer();
      cmdboo = true;
      newInputCNSLE()
      return;
    }

    // Change directory question prompt
    if(command.substring(0, 3) === "cd "){
      readLineCNSLE("cd ", command)
      cmdboo = true;
    }

    // Remove question prompt
    if(command.substring(0, 4) === "rmv " || command.substring(0, 4) === "del "){
      console.log(remove(command.substring(4, command.length)));
      cmdboo = true;
    }
    // Find question prompt
    if(command.substring(0, 5) === "find "){
      console.log(findItemMain(command.substring(5, command.length)));
      cmdboo = true;
      newInputCNSLE();
      return;
    }
    // Checks if it makes new file
    if(command == "mkdir" ||command == "mkdir " || command == "write"||command ==  "write "){
      cmdboo = true;
      console.log("|| Type name");
    }else{
      newFile(command);
    }
    // Read question prompt
    if(command.substring(0, 5) === "read "){
      console.log(readData(command.substring(5, command.length)));
      cmdboo = true;
      newInputCNSLE();
      return;
    }
    // Decrypt question prompt;
    if(command.substring(0, 8) === "decrypt "){
        console.log(decryptData(command.substring(8, command.length)));
        cmdboo = true;
        newInputCNSLE();
        return;
    }
    if(cmdboo == true){}else{
      result = readLineCNSLE(command);
    }
    if(result == undefined){}else{
      console.log(result);
    }
    newInputCNSLE();
  });
}

newFile = function(currentCMD){
    // When making file
    if(currentCMD.substring(0,6) === "mkdir " || currentCMD.substring(0,6) === "write "){
      // Check current commands for correct filetypes
      if(currentCMD.substring(0,6) === "mkdir "){fileType = "folder"}
      else if(currentCMD.substring(0,6) === "write "){fileType = "txt"}
      cmdboo = true;
      // Ask the questions
      // Applies name
      fileName = currentCMD.substring(6,currentCMD.length);
      // if user wants to write info
      if(currentCMD.substring(0,6) === "write "){
        readline.question(`Type contents:\n`, contents => {
          global.rootSecond = global.root
          global.root = global.root;
          global.rootText = global.root + ">";
          // Applies contents
          fileData = contents;
          readline.question(`Do you want to crypt the text? y/n `, question => {
            question=question.toLowerCase(); 

            if(question === "y" || question === "yes"){
              writeData(fileType, fileName, fileData, currentCMD, true);
              newInputCNSLE();
            }else if(question === "n" || question === "no"){
              writeData(fileType, fileName, fileData, currentCMD, false);
              readLineCNSLE("cd..")
              readLineCNSLE("cd " + fileName)
              newInputCNSLE();
            }
          });
          // Call newInputCNSLE() after writeData completes
          newInputCNSLE();
        });
      } else {
        global.rootSecond = global.root
        global.root = global.root + fileName + "/";
        global.rootText = global.root + ">";
        fileData;
        writeData(fileType, fileName, fileData, currentCMD);
        // Call newInputCNSLE() after writeData completes
        newInputCNSLE();
      }
    }else{
      // Return if file making cmd wasn't used
      return;
    }
    newInputCNSLE();
    // Close console reading
    // readline.close();
  };
  
  

// On start, start console
newInputCNSLE();
a = function(){newInputCNSLE}
module.exports = {
  newInputCNSLE: newInputCNSLE,
  a: a
};