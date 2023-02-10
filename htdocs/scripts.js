function addSourceToVideo(video, src, type) {
    while (video.firstChild) {
        video.removeChild(video.firstChild);
    }

    const source = document.createElement('source');

    source.src = `http://${window.location.host}/${src}`;
    source.type = type;
    video.appendChild(source);
}

function init(){
    const Http = new XMLHttpRequest();
    Http.onreadystatechange = function() {
        if (this.readyState === 4 && this.status === 200) {
            const obj = JSON.parse(Http.responseText);

            const container = document.getElementById("slide-out");
            for (let i in obj.files){
                const a = document.createElement('a');
                a.setAttribute('data-file', obj.files[i]);
                a.setAttribute('class', 'truncate sidenav-close white-text');
                a.setAttribute('href', '#!');
                const l = obj.files[i].lastIndexOf("/");
                a.appendChild(document.createTextNode(obj.files[i].substr(l+1)));

                a.onclick = function(){
                    const file = this.getAttribute('data-file');
                    const video = document.getElementById('video');
                    const n = file.lastIndexOf(".");
                    video.autoplay = true;
                    video.muted = false;
                    video.loop = true;
                    video.playsinline = true;
                    video.playsInline = true;
                    video.controls = true;
                    video.name = file.substr(0, n);

                    addSourceToVideo(video, file, "video/"+file.substr(n+1));
                    video.load();
                    video.play();
                };

                const li = document.createElement('li');
                li.appendChild(a);
                container.appendChild(li);
            }
        }
    };
    Http.open("GET", '/data');
    Http.send();
}
