# iipImage
Containerized IIP with authorization

This version only requires that any authorization header is provided.

future plans include checking validity and permissions for routes given.

## building and running

docker build . -t iipauth

docker run iipauth -d -p 4010:4010
