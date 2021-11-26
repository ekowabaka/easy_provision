var apsList;
var head;
var foot;
var offsetHeight;
var networks = [];
var activeNetwork;
var passwordInput;;

function getPassword(network) {
    return function() {
        document.querySelectorAll(".network-name").forEach(x => x.innerHTML = network.ssid);
        modal.open("#password-window")
        activeNetwork = network;
    }
}

function pollConnectionStatus() {
    fetch("/api/status")
        .then(response => response.json())
        .then(data => {
            if(data.status === "connected") {
                window.location.href = "/portal_connected.html";
            } else if(data.status === "failed") {
                modal.close();
                modal.open("#failed-window");
            } else {
                setTimeout(pollConnectionStatus, 1000);
            }
        })
}

function startConnection(e) {
    e.preventDefault();
    e.stopPropagation();

    modal.close();
    modal.open("#connecting-window", false);

    fetch("/api/connect", {
        method: "POST",
        headers: {"Content-Type": "application/json"},
        body: JSON.stringify(
            {
                "ssid": activeNetwork.ssid, 
                "password": passwordInput.value
            })
    })
    .then(response => response.json())
    .then(data => {
        if(data === true) {
            pollConnectionStatus()
        } else {
            modal.close();
            modal.open("#password-window")    
        }
    })    
}

function showPasswordWindow(e) {
    passwordInput = e.detail.modal.querySelector("input[type='password']")
    passwordInput.focus();
    e.detail.modal.querySelector(".button").addEventListener("mouseup", startConnection);
}

function refresh() {
    apsList.innerHTML = "<div style='text-align:center;padding-top:50px'><img class='spinner' src='/portal_sync.svg' /></div>";
    fetch("/api/scan")
        .then(response => response.json())
        .then(data => {
            networks = data;
            apsList.innerHTML = "";
            data.forEach(ap => {
                let element = document.createElement("div");
                let rssiLevel = Math.round(((ap.rssi + 100) / 70) * 4)

                element.className = "ap";
                auth = ap.auth > 0 ? "<img class='ap-lock' src='/portal_lock.svg'>" : "";
                element.innerHTML = `<span class="ap-ssid">${ap.ssid}</span>${auth}<img class="ap-signal" src="/portal_sig_${rssiLevel}.svg" />`;
                element.addEventListener('click', getPassword(ap))
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
    document.getElementById("password-window").addEventListener("shown", showPasswordWindow);
    document.getElementById("failed-window").addEventListener("shown", showPasswordWindow);
    console.log(document.getElementById("head").offsetHeight, document.getElementById("foot").offsetHeight);
    resize();
    refresh();
}

window.addEventListener("load", init);

/**
 * A simple object for presenting modal windows.
 */
var modal = new (function () {
    let openModal = {};

    this.open = function (description, hasCloseButton) {
        let object = document.querySelector(description);
        let backdrop = document.createElement('div');
        backdrop.classList.add('modal-backdrop');

        let close = document.createElement('div');
        let modal = object.cloneNode(true);
        let top = 200;
        let width = Math.min(window.innerWidth - 50, 500);
        let left = (window.innerWidth / 2) - (width / 2);

        let shownEvent = new CustomEvent('shown', { detail: { modal: modal } });

        object.parentNode.removeChild(object);
        modal.classList.add('modal-wrapper');

        if(hasCloseButton!==false) {
            modal.insertBefore(close, modal.firstChild);
        }
        modal.style.width = width + 'px';
        modal.style.left = left + 'px';
        modal.style.top = top + 'px';
        close.classList.add('close-button');
        close.style.left = (width - 50) + 'px';        

        backdrop.appendChild(modal);
        document.body.appendChild(backdrop);
        backdrop.style.display = 'block';
        modal.style.display = 'block';
        object.dispatchEvent(shownEvent);

        close.addEventListener('click', () => this.close(modal));
        openModal = { content: object, backdrop: backdrop };
        return modal
    }

    this.close = function (modal) {
        let content = openModal['content'];
        let backdrop = openModal['backdrop'];

        backdrop.parentNode.removeChild(backdrop);
        document.body.appendChild(content);
        content.classList.add('modal');
    }
})();
