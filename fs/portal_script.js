var apsList;
var head;
var foot;
var offsetHeight;
var networks = [];
var activeNetwork;

function getPassword(network) {
    return function() {
        document.getElementById("network-name").innerHTML = network.ssid;
        modal.open("#password-window")
        activeNetwork = network;
    }
}

function modalShown() {
    let password = document.getElementById("password-input");
    password.focus();
    document.getElementById("connect-button").addEventListener("mouseup", 
        e => {
            e.preventDefault();
            e.stopPropagation();
            modal.close();
            modal.open("#connecting-window");

            fetch("/api/connect", {
                method: "POST",
                headers: {"Content-Type": "application/json"},
                body: JSON.stringify({"ssid": activeNetwork.ssid, "password": password.value})
            })
            .then(response => response.json())
            .then(data => {
                // Once we're here then the connection definitely failed.
                modal.close();
                modal.open("#password-window")
            })    
        });
}

function refresh() {
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
    document.getElementById("password-window").addEventListener("shown", modalShown);
    console.log(document.getElementById("head").offsetHeight, document.getElementById("foot").offsetHeight);
    resize();
    refresh();
}

window.addEventListener("load", init);

/**
 * A simple object for presenting modal windows.
 */
var modal = new (function () {
    let modalCount = 0;
    let openModal = {};

    this.open = function (description) {
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
        modal.insertBefore(close, modal.firstChild);
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
        //let modalData = openModals.get(getObject(modal));
        let content = openModal['content'];
        let backdrop = openModal['backdrop'];

        backdrop.parentNode.removeChild(backdrop);
        document.body.appendChild(content);
        content.classList.add('modal');
    }
})();
