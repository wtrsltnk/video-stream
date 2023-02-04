function addSourceToVideo(video, src, type) {
    while (video.firstChild) {
        video.removeChild(video.firstChild);
    }

    var source = document.createElement('source');

    source.src = `http://${window.location.host}/${src}`;
    source.type = type;
    video.appendChild(source);
}

function init(){
    const Http = new XMLHttpRequest();
    Http.onreadystatechange = function() {
        if (this.readyState === 4 && this.status === 200) {
            var obj = JSON.parse(Http.responseText);

            var container = document.getElementById("slide-out");
            for (var i in obj.files){
                const li = document.createElement('li');
                const a = document.createElement('a');
                a.setAttribute('data-file', obj.files[i]);
                a.setAttribute('class', 'truncate sidenav-close');
                a.setAttribute('href', '#!');

                a.onclick = function(){
                    var file = this.getAttribute('data-file');
                    var video = document.getElementById('video');
                    var n = file.lastIndexOf(".");
                    addSourceToVideo(video, file, "video/"+file.substr(n+1));
                    video.autoplay = true;
                    video.muted = false;
                    video.loop = true;
                    video.playsinline = true;
                    video.playsInline = true;
                    video.controls = true;
                    video.name = file.substr(0, n);
                    video.load();

                    video.play();
                };
                const l = obj.files[i].lastIndexOf("/");
                a.appendChild(document.createTextNode(obj.files[i].substr(l+1)));
                li.appendChild(a);
                container.appendChild(li);
            }
        }
    };
    Http.open("GET", '/data');
    Http.send();
}
