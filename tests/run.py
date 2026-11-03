#!/usr/bin/env python3
"""Exercise real firmware routines with host peripherals, plus the EEPROM codec."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / 'ATS_EX/ATS_EX.ino').read_text()
FUNCTIONS = '''setup isSSB encoderButtonPressed getSteps getLastStep amSeekSpacing frequencyInBand bandFrequenciesValid
ssbFrequencyKHzFloor ssbFrequencyKHzCeil ssbRoundedFrequencyKHz
simpleEvent volumeEvent bandEvent
rotaryEncoder updateSSBCutoffFilter saveAllReceiverInformation readAllReceiverInformation
resetEepromDelay takeEncoderStep showFrequencySeek checkStopSeeking doSeek showBandTag showCharge showStep showVolume showBandwidth showModulation
switchSettings getNextSWSuBband bandSwitch applyBandConfiguration doStep updateBFO
doVolume doSwitchLogic agcSetFunc doAttenuation doSoftMute doBrightness doSSBAVC
doAvc doSync doDeEmp doSWUnits doSSBSoftMuteMode doCutoffFilter
doBFOCalibration doFreqUnits doScanSwitch doCWSwitch doBandwidthLogic doBandwidth
switchCommand clampSSBBand doFrequencyTune resetLowerLine doFrequencyTuneSSB
handleButtons loop'''.split()
if __name__ == '__main__':
    compiler = os.environ.get('CXX', 'c++')
    with tempfile.TemporaryDirectory(prefix='ats-host-tests-') as folder:
        folder = Path(folder)
        for rds in (1, 0):
            names = FUNCTIONS + (['setRDSConfig', 'doRDSErrorLevel', 'doRDS', 'showRDS'] if rds else [])
            definitions, prototypes = [], []
            for name in names:
                match = re.search(r'^\w+ ' + name + r'\([^\n]*\)\n\{', SOURCE, re.M)
                if not match:
                    raise RuntimeError('Missing firmware routine: ' + name)
                # Selected routines have balanced braces on source lines; ignore comments/strings.
                depth = 1
                end = match.end()
                for line in SOURCE[end:].splitlines(keepends=True):
                    clean = re.sub(r'"(?:\\.|[^"\\])*"|//[^\n]*', '', line)
                    depth += clean.count('{') - clean.count('}')
                    end += len(line)
                    if not depth:
                        break
                definition = SOURCE[match.start():end]
                prototypes.append(definition.split('\n', 1)[0] + ';')
                definitions.append(definition)
            (folder / 'FirmwareFunctions.inc').write_text('\n'.join(prototypes + definitions))
            executable = folder / f'firmware-test-rds{rds}'
            command = [compiler, '-std=gnu++11', '-Wall', '-Wextra', '-Werror',
                       '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                       f'-DUSE_RDS={rds}', '-I' + str(ROOT / 'tests/support'),
                       '-I' + str(ROOT / 'ATS_EX'), '-I' + str(folder),
                       str(ROOT / 'tests/firmware_test.cpp'),
                       str(ROOT / 'ATS_EX/SimpleButton.cpp'), str(ROOT / 'ATS_EX/Rotary.cpp'),
                       '-o', str(executable)]
            subprocess.run(command, check=True)
            subprocess.run([str(executable)], check=True)
