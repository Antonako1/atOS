const fs = require('fs');

const moduleATKRYPT= require("./AT-Krypt");
const atKrypt = moduleATKRYPT.atKrypt;
const atDekrypt = moduleATKRYPT.atDekrypt;

writeData = function(fileType, fileName, fileData, crntCMD, crypt){
  global.data = fs.readFileSync('files/file.json', 'utf8');
  global.data = JSON.parse(global.data);
  let objecti;
  // Read existing data from file
  

if(crntCMD === "mkdir"){
  objecti = {
    name: fileName,
    path: global.root,
    filetype: fileType,
    filedata: [
    ]
  };
}else if(crntCMD === "write"){
  if(crypt == true){
    fileData = atKrypt(fileData, 1)
    fileName = fileName + ".cr"
  }else{
    fileName = fileName + ".txt"
  }
  objecti = {
    name: fileName,
    path: global.root + fileName,
    filetype: fileType,
    text: fileData
  };
}
// Pushes new files to files.json
function pushToPath(json, path, object) {
  if(json == null){
  }else{
    if (json.path === path) {
      json.filedata.push(object);
    } else if (json.filedata) { // add this check
      json.filedata.forEach(subfolder => {
        pushToPath(subfolder, path, object);
      });
    }
  }
}

// Find the node with path 'root/test' and push objecti to its filedata array
pushToPath(global.data, global.rootSecond, objecti);


  // data.filedata.push(objecti)
  // Append new file to filedata object with correct rootpos

  // Write updated JSON string to file
  const jsonString = JSON.stringify(global.data, null, 2);
  fs.writeFileSync('files/file.json', jsonString);
};

module.exports = {
    writeData: writeData
};