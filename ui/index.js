// Make the background square pattern
let teksti = "";
let t = "";
for (let i = 0; i < 16; i++) {
    t += "<tr>";
    for (let j = 0; j < 16; j++) {
        teksti = '<td id="'+i+"."+j+'"></td>'; 
        t += teksti;
    }
    t += "</tr>";
}
document.getElementById("backgroundMain").innerHTML = t;

// Event listener for the td icon presses
