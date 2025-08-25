#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import logging
import os
import subprocess
import sys
from typing import Union

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)
stream_handler = logging.StreamHandler()
stream_handler.setLevel(logging.INFO)
stream_handler.setFormatter(logging.Formatter('[%(levelname)s] %(funcName)s:%(lineno)s: %(message)s'))
logger.addHandler(stream_handler)

key_list = [
    'PlStorageDataDpsURL',
    'PlStorageDataCommonName',
    'PlStorageDataDpsScopeID',
    'PlStorageDataProjectID',
    'PlStorageDataRegisterToken',
    'PlStorageDataWiFiSSID',
    'PlStorageDataWiFiPassword',
    'PlStorageDataWiFiEncryption',
    'PlStorageDataIPAddress',
    'PlStorageDataSubnetMask',
    'PlStorageDataGateway',
    'PlStorageDataDNS',
    'PlStorageDataIPMethod',
    'PlStorageDataNetIfKind',
    'PlStorageDataIPv6IPAddress',
    'PlStorageDataIPv6SubnetMask',
    'PlStorageDataIPv6Gateway',
    'PlStorageDataIPv6DNS',
    'PlStorageDataWiFiApSSID',
    'PlStorageDataWiFiApPassword',
    'PlStorageDataWiFiApEncryption',
    'PlStorageDataWiFiApChannel',
    'PlStorageDataWiFiApIPAddress',
    'PlStorageDataWiFiApSubnetMask',
    'PlStorageDataWiFiApGateway',
    'PlStorageDataWiFiApDNS',
    'PlStorageDataProxyURL',
    'PlStorageDataProxyPort',
    'PlStorageDataProxyUserName',
    'PlStorageDataProxyPassword',
    'PlStorageDataEvpHubURL',
    'PlStorageDataEvpHubPort',
    'PlStorageDataEvpIotPlatform',
    'PlStorageDataPkiRootCerts',
    'PlStorageDataPkiRootCertsHash',
    'PlStorageDataEvpTls',
    'PlStorageDataDeviceManifest',
    'PlStorageDataDebugLogLevel',
    'PlStorageDataEventLogLevel',
    'PlStorageDataDebugLogDestination',
    'PlStorageDataLogFilter',
    'PlStorageDataLogUseFlash',
    'PlStorageDataStorageName',
    'PlStorageDataStorageSubDirectoryPath',
    'PlStorageDataDebugLogLevel2',
    'PlStorageDataEventLogLevel2',
    'PlStorageDataDebugLogDestination2',
    'PlStorageDataLogFilter2',
    'PlStorageDataLogUseFlash2',
    'PlStorageDataStorageName2',
    'PlStorageDataStorageSubDirectoryPath2',
    'PlStorageDataDebugLogLevel3',
    'PlStorageDataEventLogLevel3',
    'PlStorageDataDebugLogDestination3',
    'PlStorageDataLogFilter3',
    'PlStorageDataLogUseFlash3',
    'PlStorageDataStorageName3',
    'PlStorageDataStorageSubDirectoryPath3',
    'PlStorageDataDebugLogLevel4',
    'PlStorageDataEventLogLevel4',
    'PlStorageDataDebugLogDestination4',
    'PlStorageDataLogFilter4',
    'PlStorageDataLogUseFlash4',
    'PlStorageDataStorageName4',
    'PlStorageDataStorageSubDirectoryPath4',
    'PlStorageDataNTPServer',
    'PlStorageDataNTPSyncInterval',
    'PlStorageDataNTPPollingTime',
    'PlStorageDataSkipModeSettings',
    'PlStorageDataLimitPacketTime',
    'PlStorageDataLimitRTCCorrectionValue',
    'PlStorageDataSanityLimit',
    'PlStorageDataSlewModeSettings',
    'PlStorageDataStableRTCCorrectionValue',
    'PlStorageDataStableSyncNumber',
    'PlStorageDataSystemError',
    'PlStorageDataFactoryResetFlag',
    'PlStorageDataRTCErrorDetection',
    'PlStorageDataRTCPQAParameter',
    'PlStorageDataBatteryInformation',
    'PlStorageDataRTCNetworkInformation',
    'PlStorageDataRTCConfig',
    'PlStorageDataHoursMeter',
    'PlStorageDataSAS',
    'PlStorageDataQRModeStateFlag',
    'PlStorageDataInitialSettingFlag',
    'PlStorageDataHWInfoText',
    'PlStorageDataMCULoaderVersion',
    'PlStorageDataSensorLoaderVersion',
    'PlStorageDataMCUFWLastUpdate',
    'PlStorageDataSensorLoaderLastUpdate',
    'PlStorageDataSensorFWLastUpdate',
    'PlStorageDataSensorAIModelFlashAddress',
    'PlStorageDataSensorLoaderFlashAddress',
    'PlStorageDataSensorFWFlashAddress',
    'PlStorageDataAIModelParameterSlot0',
    'PlStorageDataAIModelParameterSlot1',
    'PlStorageDataAIModelParameterSlot2',
    'PlStorageDataAIModelParameterSlot3',
    'PlStorageDataAIModelParameterSlot4',
    'PlStorageDataAIModelParameterHashSlot1',
    'PlStorageDataAIModelParameterHashSlot2',
    'PlStorageDataAIModelParameterHashSlot3',
    'PlStorageDataAIModelParameterHashSlot4',
    'PlStorageDataLMTStd',
    'PlStorageDataPreWBStd',
    'PlStorageDataGAMMAStd',
    'PlStorageDataLSCStd',
    'PlStorageDataLSCRawStd',
    'PlStorageDataDEWARPStd',
    'PlStorageDataLMTCustom',
    'PlStorageDataPreWBCustom',
    'PlStorageDataGAMMACustom',
    'PlStorageDataGAMMAAutoCustom',
    'PlStorageDataLSCCustom',
    'PlStorageDataLSCRawCustom',
    'PlStorageDataDEWARPCustom',
    'PlStorageDataAIISPAIModelParameterSlot0',
    'PlStorageDataAIISPLoaderFlashAddress',
    'PlStorageDataAIISPFWFlashAddress',
    'PlStorageDataAIISPAIModelParameterSlot1',
    'PlStorageDataAIISPAIModelParameterSlot2',
    'PlStorageDataAIISPAIModelParameterSlot3',
    'PlStorageDataAIISPAIModelParameterSlot4',
    'PlStorageDataAIModelSlotInfo',
    'PlStorageDataAIISPAIModelSlotInfo',
    'PlStorageDataFwMgrBinaryInfo0',
    'PlStorageDataFwMgrBinaryInfo1',
    'PlStorageDataFwMgrBinaryInfo2',
    'PlStorageDataFwMgrBinaryInfo3',
    'PlStorageDataFwMgrBinaryInfo4',
    'PlStorageDataFwMgrBinaryInfo5',
    'PlStorageDataFwMgrBinaryInfo6',
    'PlStorageDataFwMgrBinaryInfo7',
    'PlStorageDataFwMgrBinaryInfo8',
    'PlStorageDataFwMgrBinaryInfo9',
    'PlStorageDataFwMgrBinaryInfo10',
    'PlStorageDataFwMgrBinaryInfo11',
    'PlStorageDataFwMgrBinaryInfo12',
    'PlStorageDataFwMgrBinaryInfo13',
    'PlStorageDataFwMgrBinaryInfo14',
    'PlStorageDataFwMgrBinaryInfo15',
    'PlStorageDataFwMgrBinaryInfo16',
    'PlStorageDataFwMgrBinaryInfo17',
    'PlStorageDataFwMgrBinaryInfo18',
    'PlStorageDataFwMgrBinaryInfo19',
    'PlStorageDataFwMgrBinaryInfo20',
    'PlStorageDataFwMgrBinaryInfo21',
    'PlStorageDataFwMgrBinaryInfo22',
    'PlStorageDataFwMgrBinaryInfo23',
    'PlStorageDataFwMgrBinaryInfo24',
    'PlStorageDataFwMgrBinaryInfo25',
    'PlStorageDataFwMgrBinaryInfo26',
    'PlStorageDataFwMgrBinaryInfo27',
    'PlStorageDataFwMgrBinaryInfo28',
    'PlStorageDataFwMgrBinaryInfo29',
    'PlStorageDataFwMgrBinaryInfoMcuFirmware',
    'PlStorageDataEsfSensorConfig',
    'PlStorageDataSpiBootLoader',
    'PlStorageDataSpiBootFirmware',
    'PlStorageDataSpiBootAIModel',
    'PlStorageDataPreInstallAIModelInfo',
    'PlStorageDataPreInstallAIModel',
    'PlStorageDataInputTensorOnlyModel',
    'PlStorageDataInputTensorOnlyParam',
    'PlStorageDataExceptionFactor',
    'PlStorageDataExceptionInfo',
    'PlStorageDataEvpExceptionFactor',
    'PlStorageDataMigrationDone',
]

class MyApp(object):
    def __init__(self, args:argparse.Namespace, key:int) -> None:
        self._args = args
        self._key = key

    def main(self) -> bool:
        if self._args.value is not None:
            blob_data = self._make_blob_data()
        else:
            blob_data = self._make_blob_data_from_file()
        logger.debug(f'blob_data: {blob_data}')

        input_text = f'insert or replace into psm_data values({self._key}, X\'{blob_data}\');'
        logger.debug(f'input_text: {input_text}')

        cp = subprocess.run(['sqlite3', self._args.db_file], input=input_text, text=True,
                            stdout=subprocess.PIPE)
        if cp.returncode != 0:
            logger.error(f'failed to update DB, return code: {cp.returncode}')
            return False

        if cp.stdout:
            for line in cp.stdout.strip().splitlines():
                logger.info(f'{line}')

        return True

    def _make_blob_data(self) -> str:
        return bytes(self._args.value, 'utf-8').hex()

    def _make_blob_data_from_file(self) -> str:
        blob_data = ''

        with open(self._args.value_file, 'rb') as f:
            blob_data = f.read()

        return blob_data.hex()

def show_keys() -> None:
    print('Available keys:')
    for i in range(len(key_list)):
        print(f'  {key_list[i]} ({i})')

def show_db(db_file:Union[str,None]) -> bool:
    if db_file is None:
        logger.error('--db-file must be specified')
        return False

    if not os.path.isfile(db_file):
        logger.error(f'DB file not found: {db_file}')
        return False

    input_text = 'select key,value,typeof(value) from psm_data'
    logger.debug(f'input_text: {input_text}')

    cp = subprocess.run(['sqlite3', db_file], input=input_text, text=True,
                        stdout=subprocess.PIPE)
    if cp.returncode != 0:
        logger.error(f'failed to dump content of the DB file, return code: {cp.returncode}')
        return False

    if cp.stdout:
        for line in cp.stdout.strip().splitlines():
            logger.info(f'{line}')

    return True

def check_arguments(args:argparse.Namespace) -> bool:
    ret = True

    if args.key is None:
        logger.error('--key must be specified')
        ret = False

    value_cnt = 0
    if args.value is not None:
        value_cnt += 1
    if args.value_file is not None:
        value_cnt += 1
    if value_cnt == 0:
        logger.error('Either --value or --value-file must be specified')
        ret = False
    elif value_cnt == 2:
        logger.error('Both --value and --value-file are specified')
        ret = False

    if args.db_file is None:
        logger.error('--db-file must be specified')
        ret = False

    return ret

def check_key(key:str) -> Union[str,int,None]:
    for i in range(len(key_list)):
        if key_list[i] == key:
            return i
        if f'PlStorageData{key}' == key_list[i]:
            return i

    try:
        int_key = int(key)
    except Exception:
        int_key = None
    if int_key is not None:
        return int_key

    return None

def check_sqlite3_exists() -> bool:
    try:
        cp = subprocess.run(['sqlite3', '--version'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except FileNotFoundError:
            logger.error('sqlite3 command not found')
            return False
    if cp.returncode != 0:
        logger.error('"sqlite3 --version" command failed')
        return False

    version = cp.stdout.decode('utf-8').strip()
    logger.debug(f'sqlite3 version: {version}')

    return True

def init_db(db_file:str) -> bool:
    input_text = 'create table if not exists psm_data (key integer primary key unique,value)'
    logger.debug(f'input_text: {input_text}')

    cp = subprocess.run(['sqlite3', db_file], input=input_text, text=True,
                        stdout=subprocess.PIPE)
    if cp.returncode != 0:
        logger.error(f'failed to initialize DB file, return code: {cp.returncode}')
        return False

    if cp.stdout:
        for line in cp.stdout.strip().splitlines():
            logger.info(f'{line}')

    return True

if __name__ == '__main__':
    # For changing `max_help_position` from 24(default) to 30
    class CustomHelpFormatter(argparse.RawTextHelpFormatter):
        def __init__(self, prog, indent_increment=2, max_help_position=30, width=None):
            super().__init__(prog, indent_increment, max_help_position, width)

    parser = argparse.ArgumentParser(formatter_class=CustomHelpFormatter)

    parser.add_argument('--key', type=str, help='Key to be set')
    parser.add_argument('--value', type=str, help='Value to be set\n'
                                                  '  NOTE: Either --value or --value-file must be specified\n')
    parser.add_argument('--value-file', type=str, help='File containing value to be set\n'
                                                       '  NOTE: Either --value or --value-file must be specified\n')
    parser.add_argument('--db-file', type=str, help='DB File to be updated')

    parser.add_argument('--show-db', action='store_true', help='Show the content of the DB file')
    parser.add_argument('--show-keys', action='store_true', help='Show all available keys')

    parser.add_argument('--verbose', action='store_true', help='Enable verbose logging')
    parser.add_argument('--log-file', type=str, help='path to log file')

    args = parser.parse_args()

    if args.verbose:
        stream_handler.setLevel(logging.DEBUG)

    if args.show_keys:
        show_keys()
        sys.exit(0)

    if args.show_db:
        if not show_db(args.db_file):
            logger.error('Failed to show DB')
            sys.exit(1)

        sys.exit(0)

    if not check_arguments(args):
        parser.print_help()
        sys.exit(1)

    key = check_key(args.key)
    if key is None:
        logger.error(f'Invalid key: {args.key}')
        logger.error('Use --show-keys to see all available keys')
        sys.exit(1)

    if args.value_file is not None:
        if not os.path.isfile(args.value_file):
            logger.error(f'Value file not found: {args.value_file}')
            sys.exit(1)

    if not os.path.isfile(args.db_file):
        logger.warning(f'DB file not found: {args.db_file}, it will be created')
        if not init_db(args.db_file):
            logger.error('Failed to initialize DB')
            sys.exit(1)

    if not check_sqlite3_exists():
        sys.exit(1)

    if args.log_file is not None:
        if os.path.dirname(args.log_file) != '':
            os.makedirs(os.path.dirname(args.log_file), exist_ok=True)
        file_handler = logging.FileHandler(args.log_file, 'w')
        file_handler.setLevel(logging.DEBUG)
        file_handler.setFormatter(logging.Formatter('[%(asctime)s][%(levelname)s] %(funcName)s:%(lineno)s: %(message)s'))
        logger.addHandler(file_handler)

    app = MyApp(args, key)
    if not app.main():
        sys.exit(1)
