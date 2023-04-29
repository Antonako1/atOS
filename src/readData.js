const fs = require('fs');
const moduleATKRYPT= require("./AT-Krypt");
const atDekrypt = moduleATKRYPT.atDekrypt;

readData = function(fileName){
  let currentLocation = global.data;
  let text = readTextByName(fileName, currentLocation);
  
  if(fileName.includes(".cr")){
    text = atDekrypt(text);
  }
  return text;
}

function readTextByName(name, location) {
  for (let i in location.filedata) {
    let item = location.filedata[i];
    if (item && item.name == name) {
      if (item.filetype == "txt") {
        return item.text;
      } else {
        throw new Error("Item is not a text file or it was not found");
      }
    } else if (item && item.filetype == "folder") {
      let result = readTextByName(name, item);
      if (result) {
        return result;
      }
    }
  }
}



module.exports = {
    readData: readData
};