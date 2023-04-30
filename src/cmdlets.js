

// All different commands
const cmdlets = ["help", "ls", "mkdir", "cls", "rmv", "write", "path", "find", "cd", "read", "search", "date", "snake", "q", "tetris"];
const fs = require('fs');
const { isError } = require('underscore');
const _ = require('lodash');
// Different filetypes in .json files
global.paths = []
const moduleSNAKE = require("./snake.js");
const startGame = moduleSNAKE.startGame;

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
            function checkInsideFolder(json, path) {
                if (json.path === path) {
                for (let i = 0; i < json.filedata.length; i++) {
                    const subfolder = json.filedata[i];
                    if (subfolder.name === undefined) {
                    continue;
                    }
                    if (subfolder.filetype === "folder") {
                    console.log(subfolder.name + "/");
                    } else {
                    console.log(subfolder.name);
                    }
                }
            
                // Check if a new file was created in this folder
                if (global.newFileName && global.newFileFolder === json.path) {
                    console.log(global.newFileName);
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
            break;
        case "write":
            break;
        case "cd":
            global.rootSecond = "root/" 
            global.root = "root/";
            global.rootText = "root/" + ">";
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
              break;
        case "cd ":
            
              cmd2 = cmd2.substring(3, cmd2.length)
              function findPath(obj, searchValue) {
                var path = ""
                for (let key in obj) {
                  if (typeof obj[key] === "object") {
                    if (Array.isArray(obj[key])) { // check if value is an array
                      for (let i = 0; i < obj[key].length; i++) { // search each item in array
                        let result = findPath(obj[key][i], searchValue);
                        if (result !== "") {
                          // path = key + "[" + i + "]." + result;
                          path = key + "[" + i + "]";
                          return path;
                        }
                      }
                    } else { // search properties in object
                      let result = findPath(obj[key], searchValue);
                      if (result !== "") {
                        path = key + "." + result;
                        return path;
                      }
                    }
                  } else if (obj[key] === searchValue) {
                    return key;
                  }
                }
                return "";
              } 
              global.paths.push(findPath(global.data, cmd2))
              global.data = BuildPath();
              let path;
              
                try {
                path = global.data.path;
                } catch (error) {
                console.log('An error occurred while accessing global.data.path:', error);
                // Handle the error here
                }


              global.rootSecond = path
              global.root = global.rootSecond
              global.rootText = global.root + ">";  
              break;
        case "read":
            break;
        case "path":
            returnValueReadLine = "Current path is: " + global.root;
            break;
        case "rmv":  
            global.rootSecond = "root/" 
            global.root = "root/";
            global.rootText = "root/" + ">";
              break;
        case "date":
              let date = new Date;
              returnValueReadLine = date + " ";
              break;
        case "snake":
            startGame();
            break;
        case "q":
            console.log("------------------");
            console.log("|Shut down code 0|")
            console.log("------------------");
            process.exit(0)
            break;
        case "cls":
              console.clear();
              break;
        case "example":
              console.log(helloWorld());
              break;
        case "tetris":
              // call
              break;
        default:
            if(cmd == ""){
            }else{
                returnValueReadLine = "Unknown command. Type help for all commands"
            }
            break;
    }
    if(returnValueReadLine === "" || returnValueReadLine === " "){}else{
        return returnValueReadLine;
    }
}

function BuildPath() {
  let data = global.data;
  for (let i in global.paths) {
    const path = global.paths[i];
    prevData = data;
    data = _.get(data, path);
    if (data === undefined) {
      console.log("Error: global.data." + global.paths.slice(0, i + 1).join(".") + " is undefined");
      return prevData;
    }
  }
  return data;
}

  
  
module.exports = {
    readLineCNSLE: readLineCNSLE
};