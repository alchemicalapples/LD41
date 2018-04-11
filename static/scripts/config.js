
var EmberConfig = {
    display: {
        width: 1280,
        height: 720
    },
    username: (new URL(location).searchParams.get("username")),
    server: {
        address: "www.pickandblade.com",
        port: 29500
    }
};
