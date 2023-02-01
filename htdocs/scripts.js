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

            var div_files = document.getElementById("div_files");
            for (var i in obj.files){
                var a = document.createElement("button");
                a.setAttribute("data-file", obj.files[i]);
                a.onclick = function(){
                    var file = this.getAttribute('data-file');
                    console.log(window.location.host, file);
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
                var l = obj.files[i].lastIndexOf("/");
                a.appendChild(document.createTextNode(obj.files[i].substr(l+1)));
                div_files.appendChild(a);
            }
        }
    };
    Http.open("GET", '/data');
    Http.send();
}
