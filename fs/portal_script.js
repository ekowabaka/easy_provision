function init() {
    fetch("/api/scan")
        .then(response => response.json())
        .then(data => { 
            data.forEach(ap => {
                let element = document.createElement("div");
                element.className = "ap";
                element.innerHTML = `<div class="ap-ssid">${ap.ssid}</div><div class="ap-rssi">${ap.rssi}</div>`;
                document.getElementById("aps").appendChild(element);
            })
        })
}

document.addEventListener("DOMContentLoaded", init);