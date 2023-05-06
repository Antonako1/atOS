

const fs = require('fs');
const _ = require('lodash');
  // All different commands
  const cmdlets = ["help", "ls", "mkdir", "cls", "rmv", "write", "path", "find", "cd", "read", "decrypt", "find", "date", "snake", "q", "tetris", "timer", "timer check"];
// Different filetypes in .json files
global.paths = []

// const tetris = require("./tetris/tetris.js");
// const startGameTetris = tetris.startGameTetris;

const moduleSnake = require("./runSnake");
const runSnake = moduleSnake.runSnake;
// Example
const example = require("./example.js");
const helloWorld = example.helloWorld;


// String returned by readLineCNSLE()
let returnValueReadLine = String();
global.data = fs.readFileSync('files/file.json', 'utf8');
global.data = JSON.parse(global.data);
// Root set as global var
global.root = "root/"
global.rootText = "root/" + ">";
global.rootSecond = "root/"

// Receives commands
readLineCNSLE = function(cmd, cmd2){
    returnValueReadLine = " ";
    cmd = cmd.toLowerCase();
    cdm = cmd.trim();
    // Switchcase for commands
    switch (cmd) {
        case "help":
        returnValueReadLine = cmdlets;
        break;
        case "ls":
        returnValueReadLine = "";
            function checkInsideFolder(json, path) {
                if (json.path === path) {
                for (let i = 0; i < json.filedata.length; i++) {
                    const subfolder = json.filedata[i];
                    if (subfolder.name === undefined) {
                    continue;
                    }
                    if (subfolder.filetype === "folder") {
                        console.log("| " + subfolder.name + "/ ")
                    } else {
                        console.log("| " + subfolder.name + " ")
                    }
                }
            
                // Check if a new file was created in this folder
                if (global.newFileName && global.newFileFolder === json.path) {
                    console.log("| "+global.newFileName);
                }
                } else {
                for (let i = 0; i < json.filedata.length; i++) {
                    const subfolder = json.filedata[i];
                    if (subfolder.filetype === "folder") {
                    checkInsideFolder(subfolder, path);
                    }
                }
                }
            }
            checkInsideFolder(global.data, global.root);              
            break;
        case "mkdir":
            returnValueReadLine = "";
            break;
        case "write":
            returnValueReadLine = "";
            break;
        case "cd":
            global.data = fs.readFileSync('files/file.json', 'utf8');
            global.data = JSON.parse(global.data);
            global.rootSecond = "root/" 
            global.root = "root/";
            global.rootText = "root/" + ">";
            returnValueReadLine = "";
            break;
        case "cd..":
              global.paths.pop();
                let mem = 0;
                for(let i = global.root.length; i !== 0; i--){
                    let currentChar = global.root[i];
                    if(currentChar === "/"){
                        mem++
                    }
                    if(mem === 2){
                        global.root = global.root.substring(0, i+1)
                        break;
                    }
                }


                  global.rootSecond = global.root
                  global.rootText = global.root + ">";  
                  returnValueReadLine = "";
              break;
        case "cd ":
            global.paths = [];
              cmd2 = cmd2.substring(3, cmd2.length)
              function findPath(obj, searchValue, depth = Infinity) {
                var path = "";
                if (depth === 0) {// stop searching when depth reaches 0
                  return path;
                }
                for (let key in obj) {
                  if (typeof obj[key] === "object") {
                    if (Array.isArray(obj[key])) {
                      for (let i = 0; i < obj[key].length; i++) { // search each item in array
                        let result = findPath(obj[key][i], searchValue, depth - 1); // decrement depth each time
                        if (result !== "") {
                          path = key + "[" + i + "]";
                          return path;
                        }
                      }
                    } else { // search properties in object
                      let result = findPath(obj[key], searchValue, depth - 1);
                      if (result !== "") {
                        path = key + "." + result;
                        return path;
                      }
                    }
                  } else if (obj[key] === searchValue) {
                    return key;
                  }
                }
                return path;
              }
              
              let path;
                                                        // Search depth
              global.paths.push(findPath(global.data, cmd2))
              global.data = BuildPath();
              try {
                    path = global.data.path;
                } catch (error) {
                console.log('|| An error occurred while accessing global.data.path:', error);
                path = global.data;
                }

                returnValueReadLine = "";
                
              global.rootSecond = path
              global.root = global.rootSecond
              global.rootText = global.root + ">";  
              break;
        case "read":
            returnValueReadLine = "";
            break;
        case "path":
            returnValueReadLine = "";
            returnValueReadLine = "| Current path is: " + global.root;
            break;
        case "rmv " || "del ":  
        returnValueReadLine = "";
              break;
        case "date":
              let date = new Date;
              returnValueReadLine = date + " ";
              break;
        case "snake":
            // If not working in cmd, use powershell
            runSnake();
            break;
        case "q":
            console.log("-----------");
            console.log("|Shut down|");
            console.log("-----------");
            returnValueReadLine = "";
            process.exit(0)
            break;
        case "cls":
              console.clear();
              returnValueReadLine = "";

              break;
        case "example":
              console.log("| "+helloWorld());
              returnValueReadLine = "";

              break;
        case "tetris":
            // console.clear();
            // If not working, use node and add startGameTetris(); to the tetris.js file bottom
            // startGameTetris();
            console.log("| Tetris not working, run it using node instead");

              break;
        case "decrypt":
                break;
        case "find":
                break;
        default:
            if(cmd == "" || cmd == " "){
              
            }else{
              console.log("|| Unknown command. Type help for all commands");
            }
            break;
    }
    if(returnValueReadLine === "" || returnValueReadLine === " "){}else{
        return returnValueReadLine;
    }
}

function BuildPath() {
    let i = 1;
    let data = global.data;
    const path = global.paths[0];
    data = _.get(data, path);
    if (data === undefined) {
        console.log("|| Error: global.data." + global.paths.slice(0, i + 1).join(".") + " is " + data + " at cmdlets.js, buildPath()");
        global.data = fs.readFileSync('files/file.json', 'utf8');
        global.data = JSON.parse(global.data);
        data = global.data;
    }
    return data;
  }
  

  
  
module.exports = {
    readLineCNSLE: readLineCNSLE
};