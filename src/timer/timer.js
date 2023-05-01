let dateShort, dateShort2;
let newDateObj
let oldDateObj
let timeLeftObj;
let timeLeftObj2;
let timeMin;
timerMain = function(par){
    timeMin = par;
    if(isNaN(timeMin)){
        return timeMin + " is not a number"
    }
    oldDateObj = new Date;
    newDateObj = new Date(oldDateObj.getTime() + timeMin*60000);
    dateShort = newDateObj.getFullYear()+"."+newDateObj.getMonth()+"."+newDateObj.getDate() +" / "+newDateObj.getHours() +"."+ newDateObj.getMinutes() +"."+newDateObj.getSeconds();
    
    console.log("Timer set for " + timeMin + " minutes");
    console.log("Timer will be finished at " + dateShort);
}
checkTimer = function(){
    console.log("Timer will be finished at " + dateShort);
}
module.exports = {
    timerMain: timerMain,
    checkTimer: checkTimer
};
