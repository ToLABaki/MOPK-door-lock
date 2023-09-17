const char* index_html = R""""(
<!DOCTYPE html>
<html>
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>RFID Access Control</title>
        <style>
            html, body{
                margin: 0;
                padding: 0;
                background-color: black;
                color: white;
            }

            hr{
                width: 100%;
                color: white;
            }

            input[type=text]{
                background-color: black;
                color: white;
                padding: 8px 5px;
                font-size: 17px;
                width: 65%;
            }

            #deleteIDform{
                margin-top: 5%;
            }
            #deleteIDform button{
                margin-top: 10px;
            }

            .main-container{
                display: flex;
                flex-direction: column;
                justify-content: center;
                align-items: center;
                padding: 25px;
            }
            .main-container h2{
                text-align: center;
            }

            .buttons-container{
                margin-top: 5%;
                width: 100%;
                gap: 10px;
            }

            .custom-button{
                border: none;
                width: 100%;
                border-radius: 5px;
                color: white;
                opacity: 1;
                animation: mobileButtonClicked 350ms infinite;
                animation-play-state: paused;
            }

            .animate {
                animation-play-state: running;
            }

            .add-button{
                background-color: rgb(32, 88, 151);
            }

            .delete-button{
                background-color: red;
            }

            .back-button{
                background-color: limegreen;
            }

            .hide{
                display: none;
            }

            .input-container{
                display: flex;
                align-items: center;
                justify-content: space-between;
            }

            @keyframes mobileButtonClicked{
                0% {
                    opacity: 1;
                }

                50% {
                    opacity: 0.8;
                }

                100% {
                    opacity: 1;
                }
            }
        </style>
    </head>
    <body>
        <div class="main-container">
            <h2>RFID Access Control Actions</h2>
            <hr>
            <div id="controllerMessage"></div>
            <form id="deleteIDform" class="hide" onsubmit="return false;">
                <div class="input-container">
                    <label for="idNumber">ID Number:</label>
                    <input type="text" id="idNumber" inputmode="numeric" pattern="[0-9]+" required>
                </div>
                <button id="confirmDeleteRFID" type="submit" class="custom-button delete-button"><h3>OK</h3></button>
                <button id="goBack" class="custom-button back-button"><h3>Back</h3></button>
            </form>
            <div class="main-container buttons-container">
                <button id="addRFID" class="custom-button add-button"><h3>Add</h3></button>
                <button id="deleteRFID" class="custom-button delete-button"><h3>Delete</h3></button>
            </div>
        </div>
    </body>

    <script>
        const addRFID = document.querySelector("#addRFID");
        const deleteRFID = document.querySelector("#deleteRFID");
        const goBack = document.querySelector("#goBack");
        const webSocket = new WebSocket("ws://192.168.4.1/rfid/controller");

        const confirmDeleteRFID = document.querySelector("#confirmDeleteRFID");
        const controllerMessage = document.querySelector("#controllerMessage");
        const buttonsContainer = document.querySelector(".buttons-container");
        const deleteIDform = document.querySelector("#deleteIDform");

        addRFID.addEventListener("click", () => {
            sendActionMessage("ADD:");
            addRFID.classList.add("animate");
            setTimeout(() => {
                addRFID.classList.remove("animate");
            }, 300);
        });

        deleteRFID.addEventListener("click", () => {
            buttonsContainer.classList.add("hide");
            deleteIDform.classList.remove("hide");
            controllerMessage.innerHTML = '';
        });

        goBack.addEventListener("click", (e) => {
            e.preventDefault();
            deleteIDform.classList.add("hide");
            buttonsContainer.classList.remove("hide");
            controllerMessage.innerHTML = ''
        });

        confirmDeleteRFID.addEventListener("click", () => {
            let idNumber = document.querySelector("#idNumber").value;
            confirmDeleteRFID.classList.add("animate");
            setTimeout(() => {
                confirmDeleteRFID.classList.remove("animate");
            }, 300);
            sendActionMessage(`DELETE:${idNumber}`);
        });

        webSocket.onmessage = function(event){
            controllerMessage.innerHTML = `<h4>${event.data}</h4>`;
        };

        function sendActionMessage(message){
            try{
                webSocket.send(message);
            }catch(error){
                alert(error);
            }
        }
    </script>
</html>
)"""";
