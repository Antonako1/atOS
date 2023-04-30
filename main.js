// Imports consoleReadline and all others
const moduleCMDLETS = require("./src/cmdlets.js");
const readLineCNSLE = moduleCMDLETS.readLineCNSLE;

const moduleWRITEDATA = require("./src/writeData.js");
const writeData = moduleWRITEDATA.writeData; 

const moduleREADDATA= require("./src/readData.js");
const readData = moduleREADDATA.readData;

const moduleREMOVE= require("./src/remove.js");
const remove = moduleREMOVE.remove;

let fileType, fileName, fileData, fileToRead, result;
// Start and start animation
console.log("Starting atOS");
let count = 0;
const intervalId = setInterval(() => {
  process.stdout.write('\r[' + '█'.repeat(count) + ' '.repeat(20 - count) + ']');
  count++;
  if (count > 20) {
    clearInterval(intervalId);
    console.log("\n");
    console.log("             d8     ,88~-_   ,d88~~\\ ");
console.log("  /~~~8e  _d88__  d888   \\  8888    ");
console.log("      88b  888   88888    | `Y88b   ");
console.log(" e88~-888  888   88888    |  `Y88b, ");
console.log("C888  888  888    Y888   /     8888 ");
console.log(" \"88_-888  \"88_/   `88_-~   \\__88P' ");
console.log("\n atOS is now running, press backspace to continue");
console.log("type help for list of commands");                            
    }
  }, 100);

/*
Normal version of root, if it gets deleted
{
  "name": "root",
  "filetype": "folder",
  "path": "root/",
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
    // Identify commands
    if(command.substring(0, 3) === "cd "){
      readLineCNSLE("cd ", command)
    }else{
      result = readLineCNSLE(command);
    }
    if(result == undefined){}else{
      console.log(result);
    }
    if(command === "rmv"){
      readline.question("File to be deleted: ", file => {
        console.log(remove(file));
        newInputCNSLE();
      })
      return;
    }
    // Checks if it makes new file
    newFile(command)
    if(command === "read"){
      readline.question(`Filename: `, fileToRead => {
        console.log(readData(fileToRead));
        newInputCNSLE();
      });
      return;
    }
    newInputCNSLE();
  });
}

newFile = function(currentCMD){
    // When making file
    if(currentCMD === "mkdir" || currentCMD === "write"){
      // Check current commands for correct filetypes
      if(currentCMD === "mkdir"){fileType = "folder"}
      else if(currentCMD === "write"){fileType = "txt"}
      // Ask the questions
      readline.question(`Name for new ${fileType}: `, name => {
        // Applies name
        fileName = name;
        console.log(`Filename set to ${fileName}`);
        // if user wants to write info
        if(currentCMD === "write"){
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
                console.log(" GLOVAL ROOT: " + global.root);
                console.log(" GLOVAL ROOT 2 : " + global.rootSecond);
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
      });
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