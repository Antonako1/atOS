const fs = require('fs');

remove = function(cmd){
    let nameOfFile = cmd;
    if(nameOfFile === "root" || nameOfFile === "root/"){
        return "Bruhh"
    }else{
        let tulokset = deleteOBJ(nameOfFile, global.data)
        fs.writeFileSync('files/file.json', JSON.stringify(global.data, null, 2));
        return tulokset;
    }
}

function deleteOBJ(name, location) {
    for (let i in location.filedata) {
        let item = location.filedata[i];
        if (item && item.name == name) {
            location.filedata.splice(i, 1);
            return name + " was deleted with success";
        }
    }
    return "No such file found"
}

module.exports = {
    remove: remove,
};
