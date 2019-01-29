function addSourceToVideo(video, src, type) {
    while (video.firstChild) {
        video.removeChild(video.firstChild);
    }

    var source = document.createElement('source');

    source.src = src;
    source.type = type;
    video.appendChild(source);
}

function init(){
    const Http = new XMLHttpRequest();
    Http.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            var obj = JSON.parse(Http.responseText);

            var div_files = document.getElementById("div_files");
            for (var i in obj.files){
                var a = document.createElement("a");
                a.setAttribute("href", obj.files[i]);
                a.onclick = function(){
                    var file = this.getAttribute('href');
                    var video = document.getElementById("video");
                    var n = file.lastIndexOf(".");
                    addSourceToVideo(video, file, "video/"+file.substr(n+1));
                    video.load();
                    video.play();
                    return false;
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
