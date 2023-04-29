const fs = require('fs');

remove = function(cmd){
    if(cmd === "root" || cmd === "root/"){
        return "Bruhh"
    }else{
        let tulokset = deleteOBJ(cmd, data)
        fs.writeFileSync('files/file.json', JSON.stringify(global.data, null, 2));
        return tulokset;
    }
}

function deleteOBJ(name, location) {
    for (var i in location.filedata) {
        var item = location.filedata[i];
        if (item && item.name == name) {
            location.filedata.splice(i, 1);
            return name + " was deleted with success";
        }else{
            return "No such file found"
        }
    }
}

module.exports = {
    remove: remove,
};
