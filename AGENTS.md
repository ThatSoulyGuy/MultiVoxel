The way the project works is that it's a server-client connection techdemo. You will need to open two instances of the application, one should be the server and the other should be the client. To test locally, run the application, specify a server, and enter a port. Then, run the application again, specify we are a client, and input the current ip address and the port you entered for the server.

Coding guidelines:

Keep everything header-only
Braces should look like

statement()
{

}

and not 

statement() {

}


Do not abbreviate variable, function, namespace, or class names you create.
Put spacing between statements that do not go together.
