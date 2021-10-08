// qlabs/learn_weblayer/echo/echo.mojom.js is auto generated by mojom_bindings_generator.py, do not edit

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

(function() {
  var mojomId = 'qlabs/learn_weblayer/echo/echo.mojom';
  if (mojo.internal.isMojomLoaded(mojomId)) {
    console.warn('The following mojom is loaded multiple times: ' + mojomId);
    return;
  }
  mojo.internal.markMojomLoaded(mojomId);
  var bindings = mojo;
  var associatedBindings = mojo;
  var codec = mojo.internal;
  var validator = mojo.internal;

  var exports = mojo.internal.exposeNamespace('echo.mojom');



  function Echo_Execute_Params(values) {
    this.initDefaults_();
    this.initFields_(values);
  }


  Echo_Execute_Params.prototype.initDefaults_ = function() {
    this.request = null;
  };
  Echo_Execute_Params.prototype.initFields_ = function(fields) {
    for(var field in fields) {
        if (this.hasOwnProperty(field))
          this[field] = fields[field];
    }
  };

  Echo_Execute_Params.validate = function(messageValidator, offset) {
    var err;
    err = messageValidator.validateStructHeader(offset, codec.kStructHeaderSize);
    if (err !== validator.validationError.NONE)
        return err;

    var kVersionSizes = [
      {version: 0, numBytes: 16}
    ];
    err = messageValidator.validateStructVersion(offset, kVersionSizes);
    if (err !== validator.validationError.NONE)
        return err;


    // validate Echo_Execute_Params.request
    err = messageValidator.validateStringPointer(offset + codec.kStructHeaderSize + 0, false)
    if (err !== validator.validationError.NONE)
        return err;

    return validator.validationError.NONE;
  };

  Echo_Execute_Params.encodedSize = codec.kStructHeaderSize + 8;

  Echo_Execute_Params.decode = function(decoder) {
    var packed;
    var val = new Echo_Execute_Params();
    var numberOfBytes = decoder.readUint32();
    var version = decoder.readUint32();
    val.request =
        decoder.decodeStruct(codec.String);
    return val;
  };

  Echo_Execute_Params.encode = function(encoder, val) {
    var packed;
    encoder.writeUint32(Echo_Execute_Params.encodedSize);
    encoder.writeUint32(0);
    encoder.encodeStruct(codec.String, val.request);
  };
  function Echo_Execute_ResponseParams(values) {
    this.initDefaults_();
    this.initFields_(values);
  }


  Echo_Execute_ResponseParams.prototype.initDefaults_ = function() {
    this.result = null;
  };
  Echo_Execute_ResponseParams.prototype.initFields_ = function(fields) {
    for(var field in fields) {
        if (this.hasOwnProperty(field))
          this[field] = fields[field];
    }
  };

  Echo_Execute_ResponseParams.validate = function(messageValidator, offset) {
    var err;
    err = messageValidator.validateStructHeader(offset, codec.kStructHeaderSize);
    if (err !== validator.validationError.NONE)
        return err;

    var kVersionSizes = [
      {version: 0, numBytes: 16}
    ];
    err = messageValidator.validateStructVersion(offset, kVersionSizes);
    if (err !== validator.validationError.NONE)
        return err;


    // validate Echo_Execute_ResponseParams.result
    err = messageValidator.validateStringPointer(offset + codec.kStructHeaderSize + 0, false)
    if (err !== validator.validationError.NONE)
        return err;

    return validator.validationError.NONE;
  };

  Echo_Execute_ResponseParams.encodedSize = codec.kStructHeaderSize + 8;

  Echo_Execute_ResponseParams.decode = function(decoder) {
    var packed;
    var val = new Echo_Execute_ResponseParams();
    var numberOfBytes = decoder.readUint32();
    var version = decoder.readUint32();
    val.result =
        decoder.decodeStruct(codec.String);
    return val;
  };

  Echo_Execute_ResponseParams.encode = function(encoder, val) {
    var packed;
    encoder.writeUint32(Echo_Execute_ResponseParams.encodedSize);
    encoder.writeUint32(0);
    encoder.encodeStruct(codec.String, val.result);
  };
  var kEcho_Execute_Name = 531230075;

  function EchoPtr(handleOrPtrInfo) {
    this.ptr = new bindings.InterfacePtrController(Echo,
                                                   handleOrPtrInfo);
  }

  function EchoAssociatedPtr(associatedInterfacePtrInfo) {
    this.ptr = new associatedBindings.AssociatedInterfacePtrController(
        Echo, associatedInterfacePtrInfo);
  }

  EchoAssociatedPtr.prototype =
      Object.create(EchoPtr.prototype);
  EchoAssociatedPtr.prototype.constructor =
      EchoAssociatedPtr;

  function EchoProxy(receiver) {
    this.receiver_ = receiver;
  }
  EchoPtr.prototype.execute = function() {
    return EchoProxy.prototype.execute
        .apply(this.ptr.getProxy(), arguments);
  };

  EchoProxy.prototype.execute = function(request) {
    var params_ = new Echo_Execute_Params();
    params_.request = request;
    return new Promise(function(resolve, reject) {
      var builder = new codec.MessageV1Builder(
          kEcho_Execute_Name,
          codec.align(Echo_Execute_Params.encodedSize),
          codec.kMessageExpectsResponse, 0);
      builder.encodeStruct(Echo_Execute_Params, params_);
      var message = builder.finish();
      this.receiver_.acceptAndExpectResponse(message).then(function(message) {
        var reader = new codec.MessageReader(message);
        var responseParams =
            reader.decodeStruct(Echo_Execute_ResponseParams);
        resolve(responseParams);
      }).catch(function(result) {
        reject(Error("Connection error: " + result));
      });
    }.bind(this));
  };

  function EchoStub(delegate) {
    this.delegate_ = delegate;
  }
  EchoStub.prototype.execute = function(request) {
    return this.delegate_ && this.delegate_.execute && this.delegate_.execute(request);
  }

  EchoStub.prototype.accept = function(message) {
    var reader = new codec.MessageReader(message);
    switch (reader.messageName) {
    default:
      return false;
    }
  };

  EchoStub.prototype.acceptWithResponder =
      function(message, responder) {
    var reader = new codec.MessageReader(message);
    switch (reader.messageName) {
    case kEcho_Execute_Name:
      var params = reader.decodeStruct(Echo_Execute_Params);
      this.execute(params.request).then(function(response) {
        var responseParams =
            new Echo_Execute_ResponseParams();
        responseParams.result = response.result;
        var builder = new codec.MessageV1Builder(
            kEcho_Execute_Name,
            codec.align(Echo_Execute_ResponseParams.encodedSize),
            codec.kMessageIsResponse, reader.requestID);
        builder.encodeStruct(Echo_Execute_ResponseParams,
                             responseParams);
        var message = builder.finish();
        responder.accept(message);
      });
      return true;
    default:
      return false;
    }
  };

  function validateEchoRequest(messageValidator) {
    var message = messageValidator.message;
    var paramsClass = null;
    switch (message.getName()) {
      case kEcho_Execute_Name:
        if (message.expectsResponse())
          paramsClass = Echo_Execute_Params;
      break;
    }
    if (paramsClass === null)
      return validator.validationError.NONE;
    return paramsClass.validate(messageValidator, messageValidator.message.getHeaderNumBytes());
  }

  function validateEchoResponse(messageValidator) {
   var message = messageValidator.message;
   var paramsClass = null;
   switch (message.getName()) {
      case kEcho_Execute_Name:
        if (message.isResponse())
          paramsClass = Echo_Execute_ResponseParams;
        break;
    }
    if (paramsClass === null)
      return validator.validationError.NONE;
    return paramsClass.validate(messageValidator, messageValidator.message.getHeaderNumBytes());
  }

  var Echo = {
    name: 'echo.mojom.Echo',
    kVersion: 0,
    ptrClass: EchoPtr,
    proxyClass: EchoProxy,
    stubClass: EchoStub,
    validateRequest: validateEchoRequest,
    validateResponse: validateEchoResponse,
  };
  EchoStub.prototype.validator = validateEchoRequest;
  EchoProxy.prototype.validator = validateEchoResponse;
  exports.Echo = Echo;
  exports.EchoPtr = EchoPtr;
  exports.EchoAssociatedPtr = EchoAssociatedPtr;
})();