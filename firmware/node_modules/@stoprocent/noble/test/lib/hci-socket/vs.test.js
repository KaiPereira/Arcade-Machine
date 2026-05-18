const should = require('should');
const sinon = require('sinon');
const proxyquire = require('proxyquire').noCallThru();

const fakeOs = {};
const { assert } = sinon;
const vendorSpecific = proxyquire('../../../lib/hci-socket/vs', { os: fakeOs });

describe('hci-socket vs', () => {
  afterEach(() => {
    sinon.reset();
  });

  describe('parseAddress', () => {
    it('should convert to Buffer', () => {
      assert.match(vendorSpecific.setAddressCmd(0, '00:11:22:33:44:55').slice(3), Buffer.from([0x55, 0x44, 0x33, 0x22, 0x11, 0x00]));
    });

    it('should not convert to Buffer and throw an Error', () => {
      should.throws(function () {
        vendorSpecific.setAddressCmd(0, '00:11:22:33:44');
      });
    });
  });
});
