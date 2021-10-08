import * as xxx from './echo.mojom.m.js'

window.document.addEventListener('DOMContentLoaded', async function () {
  try {
    // Setup backend mojo.
    console.log(xxx);
    const echo = xxx.Echo.getRemote();
    const { result } = await echo.execute('EchoEcho!');
    document.getElementById("greeting").textContent = result;
  } catch (e) {
    document.getElementById("greeting").textContent = e;
  }
});
