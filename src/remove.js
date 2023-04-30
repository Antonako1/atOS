const fs = require('fs');

remove = function(cmd){
    if(cmd === "root" || cmd === "root/"){
        return "Bruhh"
    }else{
        let tulokset = deleteOBJ(cmd, global.data)
        fs.writeFileSync('files/file.json', JSON.stringify(global.data, null, 2));
        return tulokset;
    }
}
// TODO korjaa
function deleteOBJ(name, location) {
    console.log('location::: ', location);
    console.log('name::: ', name);
    for (let i in location.filedata) {
        let item = location.filedata[i];
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
