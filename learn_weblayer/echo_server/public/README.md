# JS Modules

### Copied from {out-dir}/gen/mojo/public/js
- `bindings.js`


### Copied from {out-dir}/gen/qlabs/learn_weblayer/echo
- `echo.mojom.m.js` : Modified to refer './bindings.js'


## Build
```
autoninja -C {out-dir} echo_demo
```
## Run
```
{out-dir}/echo_demo --enable-blink-features=MojoJS
```
