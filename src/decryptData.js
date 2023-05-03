const moduleATKRYPT= require("./AT-Krypt");
const atDekrypt = moduleATKRYPT.atDekrypt;
decryptData = function(name){
    let currentLocation = global.data;
    let text = readTextByName1(name, currentLocation);
    text = atDekrypt(text);
    return "| "+text;
  }
  
  function readTextByName1(name, location) {
    for (let i in location.filedata) {
      let item = location.filedata[i];
      if (item && item.name == name) {
        if (item.filetype == "txt") {
          return item.text;
        } else {
          throw new Error("|| Item is not a text file or it was not found");
        }
      } else if (item && item.filetype == "folder") {
        let result = readTextByName1(name, item);
        if (result) {
          return result;
        }
      }
    }
  }

module.exports = {
    decryptData: decryptData
};