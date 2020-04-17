// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quiche/src/quic/core/crypto/certificate_view.h"

#include <memory>

#include "third_party/boringssl/src/include/openssl/base.h"
#include "third_party/boringssl/src/include/openssl/evp.h"
#include "third_party/boringssl/src/include/openssl/ssl.h"
#include "net/third_party/quiche/src/quic/platform/api/quic_ip_address.h"
#include "net/third_party/quiche/src/quic/platform/api/quic_test.h"
#include "net/third_party/quiche/src/common/platform/api/quiche_string_piece.h"

namespace quic {
namespace {

using testing::ElementsAre;

// A test certificate generated by //net/tools/quic/certs/generate-certs.sh.
constexpr char kTestCertificateRaw[] = {
    '\x30', '\x82', '\x03', '\xb4', '\x30', '\x82', '\x02', '\x9c', '\xa0',
    '\x03', '\x02', '\x01', '\x02', '\x02', '\x01', '\x01', '\x30', '\x0d',
    '\x06', '\x09', '\x2a', '\x86', '\x48', '\x86', '\xf7', '\x0d', '\x01',
    '\x01', '\x0b', '\x05', '\x00', '\x30', '\x1e', '\x31', '\x1c', '\x30',
    '\x1a', '\x06', '\x03', '\x55', '\x04', '\x03', '\x0c', '\x13', '\x51',
    '\x55', '\x49', '\x43', '\x20', '\x53', '\x65', '\x72', '\x76', '\x65',
    '\x72', '\x20', '\x52', '\x6f', '\x6f', '\x74', '\x20', '\x43', '\x41',
    '\x30', '\x1e', '\x17', '\x0d', '\x32', '\x30', '\x30', '\x31', '\x33',
    '\x30', '\x31', '\x38', '\x31', '\x33', '\x35', '\x39', '\x5a', '\x17',
    '\x0d', '\x32', '\x30', '\x30', '\x32', '\x30', '\x32', '\x31', '\x38',
    '\x31', '\x33', '\x35', '\x39', '\x5a', '\x30', '\x64', '\x31', '\x0b',
    '\x30', '\x09', '\x06', '\x03', '\x55', '\x04', '\x06', '\x13', '\x02',
    '\x55', '\x53', '\x31', '\x13', '\x30', '\x11', '\x06', '\x03', '\x55',
    '\x04', '\x08', '\x0c', '\x0a', '\x43', '\x61', '\x6c', '\x69', '\x66',
    '\x6f', '\x72', '\x6e', '\x69', '\x61', '\x31', '\x16', '\x30', '\x14',
    '\x06', '\x03', '\x55', '\x04', '\x07', '\x0c', '\x0d', '\x4d', '\x6f',
    '\x75', '\x6e', '\x74', '\x61', '\x69', '\x6e', '\x20', '\x56', '\x69',
    '\x65', '\x77', '\x31', '\x14', '\x30', '\x12', '\x06', '\x03', '\x55',
    '\x04', '\x0a', '\x0c', '\x0b', '\x51', '\x55', '\x49', '\x43', '\x20',
    '\x53', '\x65', '\x72', '\x76', '\x65', '\x72', '\x31', '\x12', '\x30',
    '\x10', '\x06', '\x03', '\x55', '\x04', '\x03', '\x0c', '\x09', '\x31',
    '\x32', '\x37', '\x2e', '\x30', '\x2e', '\x30', '\x2e', '\x31', '\x30',
    '\x82', '\x01', '\x22', '\x30', '\x0d', '\x06', '\x09', '\x2a', '\x86',
    '\x48', '\x86', '\xf7', '\x0d', '\x01', '\x01', '\x01', '\x05', '\x00',
    '\x03', '\x82', '\x01', '\x0f', '\x00', '\x30', '\x82', '\x01', '\x0a',
    '\x02', '\x82', '\x01', '\x01', '\x00', '\xc5', '\xe2', '\x51', '\x6d',
    '\x3f', '\xd6', '\x28', '\xf2', '\xad', '\x34', '\x73', '\x87', '\x64',
    '\xca', '\x33', '\x19', '\x33', '\xb7', '\x75', '\x91', '\xab', '\x31',
    '\x19', '\x2b', '\xe3', '\xa4', '\x26', '\x09', '\x29', '\x8b', '\x2d',
    '\xf7', '\x52', '\x75', '\xa7', '\x55', '\x15', '\xf0', '\x11', '\xc7',
    '\xc2', '\xc4', '\xed', '\x18', '\x1b', '\x33', '\x0b', '\x71', '\x32',
    '\xe6', '\x35', '\x89', '\xcd', '\x2d', '\x5a', '\x05', '\x57', '\x4e',
    '\xc2', '\x78', '\x75', '\x65', '\x72', '\x2d', '\x8a', '\x17', '\x83',
    '\xd6', '\x32', '\x90', '\x85', '\xf8', '\x22', '\xe2', '\x65', '\xa9',
    '\xe0', '\xa0', '\xfe', '\x19', '\xb2', '\x39', '\x2d', '\x14', '\x03',
    '\x10', '\x2f', '\xcc', '\x8b', '\x5e', '\xaa', '\x25', '\x27', '\x0d',
    '\xa3', '\x37', '\x10', '\x0c', '\x17', '\xec', '\xf0', '\x8b', '\xc5',
    '\x6b', '\xed', '\x6b', '\x5e', '\xb2', '\xe2', '\x35', '\x3e', '\x46',
    '\x3b', '\xf7', '\xf6', '\x59', '\xb1', '\xe0', '\x16', '\xa6', '\xfb',
    '\x03', '\xbf', '\x84', '\x4f', '\xce', '\x64', '\x15', '\x0d', '\x59',
    '\x99', '\xa6', '\xf0', '\x7f', '\x8a', '\x33', '\x4b', '\xbb', '\x0b',
    '\xb8', '\xf2', '\xd1', '\x27', '\x90', '\x8f', '\x38', '\xf8', '\x5a',
    '\x41', '\x82', '\x07', '\x9b', '\x0d', '\xd9', '\x52', '\xe0', '\x70',
    '\xff', '\xde', '\xda', '\xd8', '\x25', '\x4e', '\x2f', '\x2d', '\x9f',
    '\xaf', '\x92', '\x63', '\xc7', '\x42', '\xb4', '\xdc', '\x16', '\x95',
    '\x23', '\x05', '\x02', '\x6b', '\xb0', '\xe8', '\xc5', '\xfe', '\x15',
    '\x9a', '\xe8', '\x7d', '\x2f', '\xdc', '\x43', '\xf4', '\x70', '\x91',
    '\x1a', '\x93', '\xbe', '\x71', '\xaf', '\x85', '\x84', '\xdb', '\xcf',
    '\x6b', '\x5c', '\x80', '\xb2', '\xd3', '\xf3', '\x42', '\x6e', '\x24',
    '\xec', '\x2a', '\x62', '\x99', '\xc6', '\x3c', '\xe5', '\x32', '\xe5',
    '\x72', '\x37', '\x30', '\x9b', '\x0b', '\xe4', '\x06', '\xb4', '\x64',
    '\x26', '\x95', '\x59', '\xba', '\xf1', '\x53', '\x83', '\x3d', '\x99',
    '\x6d', '\xf0', '\x80', '\xe2', '\xdb', '\x6b', '\x34', '\x52', '\x06',
    '\x77', '\x3c', '\x73', '\xbe', '\xc6', '\xe3', '\xce', '\xb2', '\x11',
    '\x02', '\x03', '\x01', '\x00', '\x01', '\xa3', '\x81', '\xb6', '\x30',
    '\x81', '\xb3', '\x30', '\x0c', '\x06', '\x03', '\x55', '\x1d', '\x13',
    '\x01', '\x01', '\xff', '\x04', '\x02', '\x30', '\x00', '\x30', '\x1d',
    '\x06', '\x03', '\x55', '\x1d', '\x0e', '\x04', '\x16', '\x04', '\x14',
    '\xc8', '\x54', '\x28', '\xf6', '\xd2', '\xd5', '\x12', '\x35', '\x89',
    '\x15', '\x75', '\xb8', '\xbf', '\xdd', '\xfb', '\x4a', '\xfc', '\x6c',
    '\x89', '\xde', '\x30', '\x1f', '\x06', '\x03', '\x55', '\x1d', '\x23',
    '\x04', '\x18', '\x30', '\x16', '\x80', '\x14', '\x50', '\xe4', '\x1d',
    '\xc3', '\x1a', '\xfb', '\xfd', '\x38', '\xdd', '\xa2', '\x05', '\xfd',
    '\xc8', '\xfa', '\x57', '\x0a', '\xc1', '\x06', '\x0f', '\xae', '\x30',
    '\x1d', '\x06', '\x03', '\x55', '\x1d', '\x25', '\x04', '\x16', '\x30',
    '\x14', '\x06', '\x08', '\x2b', '\x06', '\x01', '\x05', '\x05', '\x07',
    '\x03', '\x01', '\x06', '\x08', '\x2b', '\x06', '\x01', '\x05', '\x05',
    '\x07', '\x03', '\x02', '\x30', '\x44', '\x06', '\x03', '\x55', '\x1d',
    '\x11', '\x04', '\x3d', '\x30', '\x3b', '\x82', '\x0f', '\x77', '\x77',
    '\x77', '\x2e', '\x65', '\x78', '\x61', '\x6d', '\x70', '\x6c', '\x65',
    '\x2e', '\x6f', '\x72', '\x67', '\x82', '\x10', '\x6d', '\x61', '\x69',
    '\x6c', '\x2e', '\x65', '\x78', '\x61', '\x6d', '\x70', '\x6c', '\x65',
    '\x2e', '\x6f', '\x72', '\x67', '\x82', '\x10', '\x6d', '\x61', '\x69',
    '\x6c', '\x2e', '\x65', '\x78', '\x61', '\x6d', '\x70', '\x6c', '\x65',
    '\x2e', '\x63', '\x6f', '\x6d', '\x87', '\x04', '\x7f', '\x00', '\x00',
    '\x01', '\x30', '\x0d', '\x06', '\x09', '\x2a', '\x86', '\x48', '\x86',
    '\xf7', '\x0d', '\x01', '\x01', '\x0b', '\x05', '\x00', '\x03', '\x82',
    '\x01', '\x01', '\x00', '\x45', '\x41', '\x7a', '\x68', '\xe0', '\xa7',
    '\x59', '\xa1', '\x62', '\x54', '\x73', '\x74', '\x14', '\x4f', '\xde',
    '\x9c', '\x51', '\xac', '\x25', '\x97', '\x70', '\xf7', '\x09', '\x51',
    '\x39', '\x72', '\x39', '\x3c', '\xd0', '\x31', '\xe1', '\xc3', '\x02',
    '\x91', '\x14', '\x4d', '\x8f', '\x1d', '\x31', '\xab', '\x98', '\x7e',
    '\xe6', '\xbb', '\xab', '\x6a', '\xd9', '\xc5', '\x86', '\xaa', '\x4e',
    '\x6a', '\x48', '\xe9', '\xf8', '\xd7', '\xb3', '\x1d', '\xa0', '\xc5',
    '\xe6', '\xbf', '\x4c', '\x5a', '\x9b', '\xb5', '\x78', '\x01', '\xa3',
    '\x39', '\x7b', '\x5f', '\xbc', '\xb8', '\xa7', '\xc2', '\x71', '\xb0',
    '\x7b', '\xdd', '\xa1', '\x87', '\xa6', '\x54', '\x9c', '\xf6', '\x59',
    '\x81', '\xb1', '\x2c', '\xde', '\xc5', '\x8a', '\xa2', '\x06', '\x89',
    '\xb5', '\xc1', '\x7a', '\xbe', '\x0c', '\x9f', '\x3d', '\xde', '\x81',
    '\x48', '\x53', '\x71', '\x7b', '\x8d', '\xc7', '\xea', '\x87', '\xd7',
    '\xd1', '\xda', '\x94', '\xb4', '\xc5', '\xac', '\x1e', '\x83', '\xa3',
    '\x42', '\x7d', '\xe6', '\xab', '\x3f', '\xd6', '\x1c', '\xd6', '\x65',
    '\xc3', '\x60', '\xe9', '\x76', '\x54', '\x79', '\x3f', '\xeb', '\x65',
    '\x85', '\x4f', '\x60', '\x7d', '\xbb', '\x96', '\x03', '\x54', '\x2e',
    '\xd0', '\x1b', '\xe2', '\x6c', '\x2d', '\x91', '\xae', '\x33', '\x9c',
    '\x04', '\xc4', '\x44', '\x0a', '\x7d', '\x5f', '\xbb', '\x80', '\xa2',
    '\x01', '\xbc', '\x90', '\x81', '\xa5', '\xdc', '\x4a', '\xc8', '\x77',
    '\xc9', '\x8d', '\x34', '\x17', '\xe6', '\x2a', '\x7d', '\x02', '\x1e',
    '\x32', '\x3f', '\x7d', '\xd7', '\x0c', '\x80', '\x5b', '\xc6', '\x94',
    '\x6a', '\x42', '\x36', '\x05', '\x9f', '\x9e', '\xc5', '\x85', '\x9f',
    '\x60', '\xe3', '\x72', '\x73', '\x34', '\x39', '\x44', '\x75', '\x55',
    '\x60', '\x24', '\x7a', '\x8b', '\x09', '\x74', '\x84', '\x72', '\xfd',
    '\x91', '\x68', '\x93', '\x57', '\x9e', '\x70', '\x46', '\x4d', '\xe4',
    '\x30', '\x84', '\x5f', '\x20', '\x07', '\xad', '\xfd', '\x86', '\x32',
    '\xd3', '\xfb', '\xba', '\xaf', '\xd9', '\x61', '\x14', '\x3c', '\xe0',
    '\xa1', '\xa9', '\x51', '\x51', '\x0f', '\xad', '\x60'};

constexpr quiche::QuicheStringPiece kTestCertificate(
    kTestCertificateRaw,
    sizeof(kTestCertificateRaw));

constexpr char kTestCertificatePrivateKeyRaw[] = {
    '\x30', '\x82', '\x04', '\xbc', '\x02', '\x01', '\x00', '\x30', '\x0d',
    '\x06', '\x09', '\x2a', '\x86', '\x48', '\x86', '\xf7', '\x0d', '\x01',
    '\x01', '\x01', '\x05', '\x00', '\x04', '\x82', '\x04', '\xa6', '\x30',
    '\x82', '\x04', '\xa2', '\x02', '\x01', '\x00', '\x02', '\x82', '\x01',
    '\x01', '\x00', '\xc5', '\xe2', '\x51', '\x6d', '\x3f', '\xd6', '\x28',
    '\xf2', '\xad', '\x34', '\x73', '\x87', '\x64', '\xca', '\x33', '\x19',
    '\x33', '\xb7', '\x75', '\x91', '\xab', '\x31', '\x19', '\x2b', '\xe3',
    '\xa4', '\x26', '\x09', '\x29', '\x8b', '\x2d', '\xf7', '\x52', '\x75',
    '\xa7', '\x55', '\x15', '\xf0', '\x11', '\xc7', '\xc2', '\xc4', '\xed',
    '\x18', '\x1b', '\x33', '\x0b', '\x71', '\x32', '\xe6', '\x35', '\x89',
    '\xcd', '\x2d', '\x5a', '\x05', '\x57', '\x4e', '\xc2', '\x78', '\x75',
    '\x65', '\x72', '\x2d', '\x8a', '\x17', '\x83', '\xd6', '\x32', '\x90',
    '\x85', '\xf8', '\x22', '\xe2', '\x65', '\xa9', '\xe0', '\xa0', '\xfe',
    '\x19', '\xb2', '\x39', '\x2d', '\x14', '\x03', '\x10', '\x2f', '\xcc',
    '\x8b', '\x5e', '\xaa', '\x25', '\x27', '\x0d', '\xa3', '\x37', '\x10',
    '\x0c', '\x17', '\xec', '\xf0', '\x8b', '\xc5', '\x6b', '\xed', '\x6b',
    '\x5e', '\xb2', '\xe2', '\x35', '\x3e', '\x46', '\x3b', '\xf7', '\xf6',
    '\x59', '\xb1', '\xe0', '\x16', '\xa6', '\xfb', '\x03', '\xbf', '\x84',
    '\x4f', '\xce', '\x64', '\x15', '\x0d', '\x59', '\x99', '\xa6', '\xf0',
    '\x7f', '\x8a', '\x33', '\x4b', '\xbb', '\x0b', '\xb8', '\xf2', '\xd1',
    '\x27', '\x90', '\x8f', '\x38', '\xf8', '\x5a', '\x41', '\x82', '\x07',
    '\x9b', '\x0d', '\xd9', '\x52', '\xe0', '\x70', '\xff', '\xde', '\xda',
    '\xd8', '\x25', '\x4e', '\x2f', '\x2d', '\x9f', '\xaf', '\x92', '\x63',
    '\xc7', '\x42', '\xb4', '\xdc', '\x16', '\x95', '\x23', '\x05', '\x02',
    '\x6b', '\xb0', '\xe8', '\xc5', '\xfe', '\x15', '\x9a', '\xe8', '\x7d',
    '\x2f', '\xdc', '\x43', '\xf4', '\x70', '\x91', '\x1a', '\x93', '\xbe',
    '\x71', '\xaf', '\x85', '\x84', '\xdb', '\xcf', '\x6b', '\x5c', '\x80',
    '\xb2', '\xd3', '\xf3', '\x42', '\x6e', '\x24', '\xec', '\x2a', '\x62',
    '\x99', '\xc6', '\x3c', '\xe5', '\x32', '\xe5', '\x72', '\x37', '\x30',
    '\x9b', '\x0b', '\xe4', '\x06', '\xb4', '\x64', '\x26', '\x95', '\x59',
    '\xba', '\xf1', '\x53', '\x83', '\x3d', '\x99', '\x6d', '\xf0', '\x80',
    '\xe2', '\xdb', '\x6b', '\x34', '\x52', '\x06', '\x77', '\x3c', '\x73',
    '\xbe', '\xc6', '\xe3', '\xce', '\xb2', '\x11', '\x02', '\x03', '\x01',
    '\x00', '\x01', '\x02', '\x82', '\x01', '\x00', '\x39', '\x75', '\xac',
    '\x1b', '\x43', '\x0c', '\x16', '\xbb', '\xd0', '\xdb', '\x88', '\x28',
    '\x6a', '\x75', '\xe4', '\x3c', '\x8f', '\x2d', '\xd8', '\x6f', '\xc1',
    '\xfb', '\xf1', '\xc9', '\x32', '\xc2', '\xb9', '\x60', '\xb3', '\xb5',
    '\x7c', '\x55', '\x72', '\x96', '\x43', '\x4e', '\x8b', '\x9e', '\x38',
    '\x2b', '\x7f', '\x3c', '\xdb', '\x73', '\xc2', '\x82', '\x21', '\xf2',
    '\x6e', '\xcb', '\x36', '\x04', '\x9b', '\x95', '\x6d', '\xac', '\x5b',
    '\x5b', '\xbd', '\x50', '\x69', '\x16', '\x59', '\xff', '\x2b', '\x38',
    '\x04', '\xca', '\x2f', '\xc8', '\x93', '\x7e', '\x27', '\xf3', '\x01',
    '\x7e', '\x40', '\x81', '\xbf', '\x07', '\x0b', '\x1f', '\x5b', '\x1d',
    '\x92', '\x7e', '\x22', '\xc3', '\x0c', '\x3d', '\x22', '\xbe', '\xc3',
    '\x06', '\x4c', '\xbc', '\x72', '\x66', '\x70', '\x94', '\x16', '\x8d',
    '\x1f', '\x78', '\x65', '\x6a', '\x66', '\x07', '\x1f', '\x74', '\x42',
    '\x6e', '\xf6', '\x7e', '\xdc', '\x03', '\xd3', '\x88', '\xb4', '\x4b',
    '\x2c', '\x5c', '\x3c', '\x42', '\x59', '\x42', '\x1f', '\x01', '\x13',
    '\x31', '\xc5', '\x22', '\xe7', '\x6a', '\x96', '\xf2', '\xfb', '\x66',
    '\xfe', '\xc8', '\xa1', '\x7e', '\x24', '\x96', '\x5f', '\x02', '\xee',
    '\x38', '\x21', '\xa5', '\x14', '\xd2', '\xa6', '\x35', '\x70', '\x6c',
    '\x8d', '\xa6', '\xd8', '\x2a', '\xd2', '\x45', '\x31', '\x5f', '\x67',
    '\x9e', '\x35', '\x57', '\x6a', '\xc4', '\x15', '\xe7', '\xba', '\x60',
    '\x2f', '\x8e', '\x52', '\x4e', '\xfc', '\x6f', '\xa0', '\x08', '\x91',
    '\x31', '\x71', '\x06', '\x68', '\x19', '\x48', '\xc7', '\x81', '\x0d',
    '\x5e', '\x52', '\x93', '\x57', '\xcc', '\xfe', '\x46', '\xac', '\xa9',
    '\x4f', '\xe2', '\x96', '\x4f', '\xaf', '\x12', '\xfb', '\xc2', '\x4b',
    '\xc4', '\x8d', '\x3b', '\xb0', '\x38', '\xe4', '\xbb', '\x8d', '\x19',
    '\x81', '\xe4', '\x74', '\x63', '\x9c', '\x8d', '\xaa', '\x84', '\x82',
    '\x91', '\xdf', '\xdc', '\x45', '\xf0', '\x39', '\xb2', '\xb4', '\xac',
    '\x45', '\xda', '\x3f', '\x30', '\x4d', '\x46', '\xb1', '\xe1', '\xb2',
    '\x9d', '\xdf', '\xd8', '\xc4', '\xa2', '\xef', '\xe9', '\x1a', '\x97',
    '\x79', '\x02', '\x81', '\x81', '\x00', '\xe5', '\x23', '\xb8', '\xd7',
    '\x09', '\x54', '\x54', '\x3b', '\xb6', '\x78', '\x78', '\x67', '\x57',
    '\x65', '\xc5', '\xd4', '\x74', '\xaf', '\x05', '\x4f', '\xb5', '\xc8',
    '\x8c', '\x1b', '\xd1', '\x9a', '\x2c', '\xd6', '\xe4', '\x68', '\xd1',
    '\xaf', '\x3d', '\x72', '\x42', '\x50', '\xc8', '\xdd', '\xb1', '\xee',
    '\x77', '\x52', '\xb8', '\xb1', '\x31', '\xbe', '\xf0', '\x74', '\x78',
    '\x42', '\x59', '\xea', '\x13', '\x8b', '\x82', '\x00', '\x54', '\x22',
    '\xd2', '\x0a', '\x24', '\xb0', '\x1f', '\x1e', '\x76', '\x27', '\xae',
    '\x63', '\xc6', '\x6b', '\x59', '\x28', '\x1d', '\xa0', '\x9f', '\x42',
    '\x30', '\xf1', '\xe3', '\x59', '\x1c', '\x4f', '\x31', '\x49', '\xff',
    '\x45', '\x7e', '\x6b', '\xef', '\xe9', '\x6f', '\xde', '\xaf', '\x1e',
    '\x04', '\x96', '\x61', '\x4e', '\x9f', '\x58', '\xf5', '\x0d', '\x64',
    '\x08', '\x48', '\x0a', '\xae', '\xac', '\xe4', '\x76', '\x91', '\xdd',
    '\x6e', '\x33', '\x97', '\xc5', '\x96', '\xda', '\xff', '\xbc', '\x42',
    '\x5b', '\x71', '\xb5', '\x76', '\xae', '\x01', '\xb3', '\x02', '\x81',
    '\x81', '\x00', '\xdd', '\x14', '\xa5', '\x6c', '\x89', '\x2b', '\x80',
    '\x78', '\xf6', '\xc3', '\x80', '\x4d', '\x53', '\x54', '\xb3', '\x2b',
    '\x40', '\xce', '\x98', '\x16', '\xa0', '\xbf', '\x72', '\xf1', '\xe3',
    '\xdc', '\xe9', '\x0b', '\x45', '\x23', '\x86', '\x38', '\x4c', '\x29',
    '\xf1', '\xa0', '\xe0', '\x2c', '\xfa', '\x86', '\x3f', '\x01', '\x90',
    '\xc5', '\x1b', '\x96', '\x10', '\x44', '\x84', '\xfb', '\xec', '\x3c',
    '\x74', '\x6c', '\x0d', '\xcc', '\xc3', '\xcd', '\x1b', '\x28', '\x12',
    '\xaa', '\xb4', '\x67', '\x80', '\xc8', '\xd9', '\x1b', '\x7d', '\xe7',
    '\x54', '\x39', '\x03', '\x6d', '\xba', '\xaa', '\x6f', '\xf7', '\x93',
    '\x1f', '\x94', '\x76', '\xd6', '\xab', '\x9b', '\xda', '\x3d', '\x89',
    '\x37', '\x83', '\xfe', '\x72', '\x2a', '\xbb', '\x6f', '\x36', '\xc5',
    '\xe0', '\xae', '\x65', '\xf9', '\xbb', '\xc6', '\xe2', '\x98', '\x0f',
    '\xbd', '\xf6', '\x22', '\xf8', '\x35', '\x5b', '\x99', '\xe6', '\xff',
    '\x6d', '\x6e', '\xb2', '\x92', '\x93', '\x64', '\x25', '\xc1', '\xe8',
    '\x9c', '\x6b', '\x73', '\x2b', '\x02', '\x81', '\x80', '\x13', '\x30',
    '\x1a', '\x9a', '\x67', '\x3d', '\x98', '\x90', '\x27', '\x87', '\x8f',
    '\x0d', '\x98', '\x53', '\xfd', '\x6c', '\xfd', '\x18', '\x6a', '\xe9',
    '\x71', '\xdf', '\x89', '\x5c', '\x0b', '\x01', '\x4e', '\x1f', '\xf0',
    '\xa0', '\x96', '\x6e', '\x86', '\x46', '\xbb', '\x26', '\xe8', '\xab',
    '\x27', '\xeb', '\x40', '\x32', '\xbd', '\x24', '\x99', '\x75', '\xd3',
    '\xcc', '\xed', '\x05', '\x21', '\x62', '\x68', '\xa0', '\x96', '\x12',
    '\x50', '\xf9', '\x59', '\x7d', '\x5f', '\xf5', '\x1f', '\xa5', '\xfd',
    '\x5e', '\xf5', '\x4b', '\x85', '\xa2', '\x17', '\xa5', '\x34', '\x55',
    '\xef', '\x00', '\x2b', '\xf9', '\x15', '\x80', '\xb0', '\xce', '\x30',
    '\xe2', '\x71', '\x6d', '\xf0', '\x58', '\x39', '\x8e', '\xe2', '\xbf',
    '\x53', '\x0a', '\xc0', '\x77', '\x97', '\x4e', '\x6e', '\x29', '\x94',
    '\xdb', '\xba', '\x34', '\xb7', '\x53', '\xad', '\xac', '\xec', '\xb4',
    '\xc1', '\x22', '\x39', '\xc8', '\x38', '\x3d', '\x63', '\x94', '\x93',
    '\x35', '\xc0', '\x98', '\xc7', '\xbc', '\xda', '\x63', '\x57', '\xe1',
    '\x02', '\x81', '\x80', '\x51', '\x71', '\x7c', '\xab', '\x6a', '\x30',
    '\xe3', '\x68', '\x2c', '\x87', '\xc2', '\xe9', '\x39', '\x8c', '\x97',
    '\x60', '\x94', '\xc4', '\x46', '\xd4', '\xf7', '\x2c', '\xf0', '\x1c',
    '\x5a', '\x34', '\x14', '\x89', '\xf9', '\x53', '\x67', '\xeb', '\xaf',
    '\x6b', '\x38', '\x3f', '\x6a', '\xb6', '\x47', '\x28', '\x53', '\x67',
    '\xb1', '\x3c', '\x5b', '\xb8', '\x41', '\x8f', '\xec', '\x69', '\x9e',
    '\x12', '\x7b', '\x55', '\x1f', '\x14', '\x53', '\x01', '\x69', '\x42',
    '\xae', '\xf5', '\xc1', '\xf5', '\xeb', '\x44', '\x92', '\x6e', '\x85',
    '\x48', '\x46', '\x07', '\xa6', '\xd2', '\xb2', '\x94', '\x7d', '\x20',
    '\xf8', '\x4b', '\x06', '\xf7', '\x6c', '\x87', '\xd5', '\xa7', '\x65',
    '\x49', '\xfa', '\x70', '\x9e', '\xb8', '\xd2', '\x33', '\x30', '\x7a',
    '\x3e', '\x15', '\x52', '\x49', '\xf0', '\xe1', '\x13', '\x18', '\x80',
    '\xaa', '\x33', '\xf1', '\xcb', '\xda', '\x22', '\x55', '\xf7', '\x71',
    '\x58', '\xa1', '\xa8', '\xc9', '\x12', '\x24', '\x48', '\x1d', '\x7c',
    '\xbc', '\xc3', '\x7a', '\xf5', '\xf7', '\x02', '\x81', '\x80', '\x41',
    '\x7c', '\xae', '\x6e', '\x48', '\x3f', '\xb5', '\x0b', '\x99', '\xaa',
    '\xc5', '\xea', '\x81', '\xad', '\x84', '\x6b', '\x29', '\x78', '\x4b',
    '\x18', '\xdb', '\x0e', '\xd3', '\x3e', '\x60', '\x8b', '\xef', '\x65',
    '\x4d', '\x58', '\x25', '\x3a', '\x08', '\xb5', '\x21', '\xb6', '\x61',
    '\x0c', '\xfa', '\xf0', '\x69', '\x78', '\x4e', '\x68', '\x36', '\xdb',
    '\x41', '\x4b', '\x50', '\xd8', '\xd3', '\x8e', '\x3d', '\x74', '\x80',
    '\x8e', '\xa0', '\xe6', '\xda', '\xec', '\x70', '\x89', '\x77', '\xb2',
    '\x9d', '\xd6', '\x6e', '\x0a', '\xc4', '\xbd', '\xf6', '\x9a', '\x07',
    '\x15', '\xba', '\x55', '\x9f', '\xd4', '\x4d', '\x3a', '\x0f', '\x51',
    '\x12', '\xa4', '\xd9', '\xc2', '\x98', '\x76', '\xc5', '\xb7', '\x29',
    '\x40', '\xca', '\xf4', '\xbb', '\x74', '\x2d', '\x71', '\x03', '\x4d',
    '\xe7', '\x05', '\x75', '\xc0', '\x8d', '\x96', '\x7e', '\x59', '\xa1',
    '\x8b', '\x3b', '\xa3', '\x2b', '\xa5', '\xa3', '\xc8', '\xf7', '\xd3',
    '\x3e', '\x6b', '\x2e', '\xfa', '\x4f', '\x4d', '\xe6', '\xbe', '\xd3',
    '\x59'};

constexpr quiche::QuicheStringPiece kTestCertificatePrivateKey(
    kTestCertificatePrivateKeyRaw,
    sizeof(kTestCertificatePrivateKeyRaw));

TEST(CertificateViewTest, Parse) {
  std::unique_ptr<CertificateView> view =
      CertificateView::ParseSingleCertificate(kTestCertificate);
  ASSERT_TRUE(view != nullptr);

  EXPECT_THAT(view->subject_alt_name_domains(),
              ElementsAre(quiche::QuicheStringPiece("www.example.org"),
                          quiche::QuicheStringPiece("mail.example.org"),
                          quiche::QuicheStringPiece("mail.example.com")));
  EXPECT_THAT(view->subject_alt_name_ips(),
              ElementsAre(QuicIpAddress::Loopback4()));
  EXPECT_EQ(EVP_PKEY_id(view->public_key()), EVP_PKEY_RSA);
}

TEST(CertificateViewTest, SignAndVerify) {
  std::unique_ptr<CertificatePrivateKey> key =
      CertificatePrivateKey::LoadFromDer(kTestCertificatePrivateKey);
  ASSERT_TRUE(key != nullptr);

  std::string data = "A really important message";
  std::string signature = key->Sign(data, SSL_SIGN_RSA_PSS_RSAE_SHA256);
  ASSERT_FALSE(signature.empty());

  std::unique_ptr<CertificateView> view =
      CertificateView::ParseSingleCertificate(kTestCertificate);
  ASSERT_TRUE(view != nullptr);
  EXPECT_TRUE(key->MatchesPublicKey(*view));

  EXPECT_TRUE(
      view->VerifySignature(data, signature, SSL_SIGN_RSA_PSS_RSAE_SHA256));
  EXPECT_FALSE(view->VerifySignature("An unimportant message", signature,
                                     SSL_SIGN_RSA_PSS_RSAE_SHA256));
  EXPECT_FALSE(view->VerifySignature(data, "Not a signature",
                                     SSL_SIGN_RSA_PSS_RSAE_SHA256));
}

}  // namespace
}  // namespace quic
