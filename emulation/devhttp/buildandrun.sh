docker build -t dev_http:1.0.0 .

docker run -it -d --name dev_http dev_http:1.0.0
