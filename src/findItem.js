const fs = require('fs');
let text = "";
findItemMain = function(filesName){
    let currentLocation = global.data;
    text = searchJSONForValue(filesName, currentLocation);
    if(text === false){
        text = "Item was not found. \"find\" is case sensitive"
    }
    return text;
  }
  
  function searchJSONForValue(value, data, paths = []) {
    // Loop through data and check if has property object
    for (let i in data) {
      if (data.hasOwnProperty(i)) {
        if (typeof data[i] === "object") {
          searchJSONForValue(value, data[i], paths);
          if (data[i].path && data[i][i] === value) {
            paths.push(data[i].path);
          }
        } else if (data[i] === value) {
          paths.push(data.path);
        }
      }
    }
    // if paths.length is 0, paths will be returned as false
    return paths.length > 0 ? paths : false;
  }
  
  
module.exports = {
    findItemMain: findItemMain
}