import * as qlabs from './echo.mojom.m.js';

window.document.addEventListener('DOMContentLoaded', async function () {
  try {
    const echo = qlabs.Echo.getRemote();
    const { result } = await echo.execute('Hello, Mojo!');
    document.getElementById("greeting").textContent = result;
  } catch (e) {
    document.getElementById("greeting").textContent = e;
  }
});
