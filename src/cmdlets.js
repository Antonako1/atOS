

// All different commands
const cmdlets = ["help", "ls", "mkdir", "cls", "rmv", "write", "path", "find", "cd", "read"];
const fs = require('fs');
// Different filetypes in .json files
global.paths = []
// AT-Krypt

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
            function checkInsideFolder(json) {
                for(let i=0; i<json.filedata.length; i++){
                    subfolder = json.filedata[i]
                    console.log(subfolder);
                    returnValueReadLine += subfolder.name;
                    if (subfolder.filetype === "folder") {
                        returnValueReadLine += "/ ";
                    }else{returnValueReadLine += " "}
                }
            }
            checkInsideFolder(global.data);              
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
              global.rootSecond = global.data.path
              global.root = global.data.path
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
        default:
            if(cmd == ""){
                returnValueReadLine = global.rootText;
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
    var evalPath = "";
    for (var i in global.paths) {
        if (evalPath.length == 0) {
            evalPath = String(global.paths[i])
        } else {
            evalPath += "." + String(global.paths[i])
        }
    }
    return eval("global.data." + evalPath)
}
module.exports = {
    readLineCNSLE: readLineCNSLE
};