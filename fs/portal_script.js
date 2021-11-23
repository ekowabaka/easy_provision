var apsList;
var head;
var foot;
var offsetHeight;

function refresh() {
    fetch("/api/scan")
        .then(response => response.json())
        .then(data => { 
            apsList.innerHTML = "";
            data.forEach(ap => {
                let element = document.createElement("div");
                let rssiLevel = Math.round(((ap.rssi + 100) / 70) * 4)

                element.className = "ap";
                auth = ap.auth > 0 ? "<img class='ap-lock' src='/portal_lock.svg'>" : "";
                element.innerHTML = `<span class="ap-ssid">${ap.ssid}</span>${auth}<img class="ap-signal" src="/portal_sig_${rssiLevel}.svg" />`;
                apsList.appendChild(element);
            })
        })
}

function resize() {
    let height = window.innerHeight - (head.offsetHeight + foot.offsetHeight);
    apsList.style.height = `${height}px`;
}

function init() {
    document.getElementById("refresh-button").addEventListener("click", refresh);
    window.addEventListener("resize", resize);
    apsList = document.getElementById("aps-list");
    head = document.getElementById("head");
    foot = document.getElementById("foot");
    console.log(document.getElementById("head").offsetHeight, document.getElementById("foot").offsetHeight);
    resize();
    refresh();
}

//document.addEventListener("DOMContentLoaded", init);
window.addEventListener("load", init);