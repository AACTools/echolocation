#include "stack_module_audio.h"

#include <M5Unified.h>
#include <driver/i2s.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

namespace {

constexpr uint8_t kStm32Addr = 0x33;
constexpr uint8_t kEs8388Addr = 0x10;
constexpr uint32_t kI2cSpeedHz = 400000;

constexpr uint8_t kHpModeReg = 0x10;

constexpr uint8_t kEs8388MasterMode = 0x08;
constexpr uint8_t kEs8388ChipPower = 0x02;
constexpr uint8_t kEs8388Control1 = 0x00;
constexpr uint8_t kEs8388Control2 = 0x01;
constexpr uint8_t kEs8388AdcPower = 0x03;
constexpr uint8_t kEs8388DacPower = 0x04;
constexpr uint8_t kEs8388AdcControl1 = 0x09;
constexpr uint8_t kEs8388AdcControl2 = 0x0a;
constexpr uint8_t kEs8388AdcControl3 = 0x0b;
constexpr uint8_t kEs8388AdcControl4 = 0x0c;
constexpr uint8_t kEs8388AdcControl5 = 0x0d;
constexpr uint8_t kEs8388AdcControl7 = 0x0f;
constexpr uint8_t kEs8388AdcControl8 = 0x10;
constexpr uint8_t kEs8388AdcControl9 = 0x11;
constexpr uint8_t kEs8388AdcControl10 = 0x12;
constexpr uint8_t kEs8388AdcControl11 = 0x13;
constexpr uint8_t kEs8388AdcControl12 = 0x14;
constexpr uint8_t kEs8388AdcControl13 = 0x15;
constexpr uint8_t kEs8388AdcControl14 = 0x16;
constexpr uint8_t kEs8388DacControl1 = 0x17;
constexpr uint8_t kEs8388DacControl2 = 0x18;
constexpr uint8_t kEs8388DacControl3 = 0x19;
constexpr uint8_t kEs8388DacControl4 = 0x1a;
constexpr uint8_t kEs8388DacControl5 = 0x1b;
constexpr uint8_t kEs8388DacControl16 = 0x26;
constexpr uint8_t kEs8388DacControl17 = 0x27;
constexpr uint8_t kEs8388DacControl18 = 0x28;
constexpr uint8_t kEs8388DacControl19 = 0x29;
constexpr uint8_t kEs8388DacControl20 = 0x2a;
constexpr uint8_t kEs8388DacControl21 = 0x2b;
constexpr uint8_t kEs8388DacControl24 = 0x2e;
constexpr uint8_t kEs8388DacControl25 = 0x2f;
constexpr uint8_t kEs8388DacControl26 = 0x30;
constexpr uint8_t kEs8388DacControl27 = 0x31;

constexpr uint8_t kHpModeNational = 0;
constexpr uint8_t kDacOutputOut1 = 0x30;

bool codec_ready = false;
bool i2s_active = false;
constexpr i2s_port_t kI2sPort = I2S_NUM_0;
i2s_config_t i2s_cfg = {};
QueueHandle_t play_queue = nullptr;
TaskHandle_t play_task = nullptr;
volatile bool abort_playback = false;

struct PlayJob {
  uint8_t* pcm = nullptr;
  size_t pcm_len = 0;
  uint32_t sample_rate_hz = 44100;
};

bool writeReg8(uint8_t addr, uint8_t reg, uint8_t value) {
  return M5.In_I2C.writeRegister(addr, reg, &value, 1, kI2cSpeedHz);
}

bool readReg8(uint8_t addr, uint8_t reg, uint8_t* value) {
  return M5.In_I2C.readRegister(addr, reg, value, 1, kI2cSpeedHz);
}

bool writeEs8388Reg(uint8_t reg, uint8_t value) { return writeReg8(kEs8388Addr, reg, value); }

bool readEs8388Reg(uint8_t reg, uint8_t* value) { return readReg8(kEs8388Addr, reg, value); }

bool initEs8388() {
  bool ok = true;
  ok &= writeEs8388Reg(kEs8388MasterMode, 0x00);
  ok &= writeEs8388Reg(kEs8388ChipPower, 0xFF);
  ok &= writeEs8388Reg(kEs8388DacControl21, 0x80);
  ok &= writeEs8388Reg(kEs8388Control1, 0x05);
  ok &= writeEs8388Reg(kEs8388Control2, 0x40);
  ok &= writeEs8388Reg(kEs8388AdcPower, 0x00);
  ok &= writeEs8388Reg(kEs8388AdcControl2, 0x00);
  ok &= writeEs8388Reg(kEs8388AdcControl3, 0x00);
  ok &= writeEs8388Reg(kEs8388AdcControl1, 0x88);
  ok &= writeEs8388Reg(kEs8388AdcControl4, 0x2C);
  ok &= writeEs8388Reg(kEs8388AdcControl5, 0x02);
  ok &= writeEs8388Reg(kEs8388AdcControl7, 0x28);
  ok &= writeEs8388Reg(kEs8388AdcControl8, 0x00);
  ok &= writeEs8388Reg(kEs8388AdcControl9, 0x00);
  ok &= writeEs8388Reg(kEs8388AdcControl10, 0xea);
  ok &= writeEs8388Reg(kEs8388AdcControl11, 0xC0);
  ok &= writeEs8388Reg(kEs8388AdcControl12, 0x12);
  ok &= writeEs8388Reg(kEs8388AdcControl13, 0x06);
  ok &= writeEs8388Reg(kEs8388AdcControl14, 0xC3);
  ok &= writeEs8388Reg(kEs8388DacPower, 0x3F);
  ok &= writeEs8388Reg(kEs8388DacControl1, 0x18);
  ok &= writeEs8388Reg(kEs8388DacControl2, 0x02);
  ok &= writeEs8388Reg(kEs8388DacControl3, 0x00);
  ok &= writeEs8388Reg(kEs8388DacControl4, 0x05);
  ok &= writeEs8388Reg(kEs8388DacControl5, 0x05);
  ok &= writeEs8388Reg(kEs8388DacControl16, 0x00);
  ok &= writeEs8388Reg(kEs8388DacControl17, 0xd0);
  ok &= writeEs8388Reg(kEs8388DacControl18, 0x38);
  ok &= writeEs8388Reg(kEs8388DacControl19, 0x38);
  ok &= writeEs8388Reg(kEs8388DacControl20, 0xd0);
  ok &= writeEs8388Reg(kEs8388DacControl21, 0x80);
  ok &= writeEs8388Reg(kEs8388DacControl24, 0x12);
  ok &= writeEs8388Reg(kEs8388DacControl25, 0x12);
  ok &= writeEs8388Reg(kEs8388DacControl26, 0x00);
  ok &= writeEs8388Reg(kEs8388DacControl27, 0x00);
  ok &= writeEs8388Reg(kEs8388ChipPower, 0x00);
  return ok;
}

bool setEs8388SampleRate(uint32_t hz) {
  uint8_t adc_fs = 0x02;
  uint8_t dac_fs = 0x02;
  uint8_t master = 0x00;

  if (hz <= 8000) {
    adc_fs = 0x0A;
    dac_fs = 0x0A;
  } else if (hz <= 11025) {
    adc_fs = 0x07;
    dac_fs = 0x07;
  } else if (hz <= 16000) {
    adc_fs = 0x06;
    dac_fs = 0x06;
  } else if (hz <= 24000) {
    adc_fs = 0x04;
    dac_fs = 0x04;
  } else if (hz <= 32000) {
    adc_fs = 0x03;
    dac_fs = 0x03;
  } else if (hz <= 44100) {
    adc_fs = 0x02;
    dac_fs = 0x02;
    master = 0x40;
  }

  if (i2s_active &&
      i2s_set_clk(kI2sPort, hz, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO) != ESP_OK) {
    return false;
  }

  return writeEs8388Reg(kEs8388MasterMode, master) &&
         writeEs8388Reg(kEs8388AdcControl5, adc_fs & 0x1F) &&
         writeEs8388Reg(kEs8388DacControl2, dac_fs & 0x1F);
}

void setEs8388Mute(bool mute) {
  uint8_t reg = 0;
  if (!readEs8388Reg(kEs8388DacControl3, &reg)) {
    return;
  }
  if (mute) {
    writeEs8388Reg(kEs8388DacControl3, static_cast<uint8_t>(reg | 0x02));
  } else {
    writeEs8388Reg(kEs8388DacControl3, static_cast<uint8_t>(reg & static_cast<uint8_t>(~0x02)));
  }
}

void setEs8388Volume(uint8_t volume) {
  volume = volume > 100 ? 100 : volume;
  uint8_t steps = static_cast<uint8_t>((static_cast<uint16_t>(volume) * 33 + 50) / 100);
  if (steps > 0x21) {
    steps = 0x21;
  }
  writeEs8388Reg(kEs8388DacControl24, steps);
  writeEs8388Reg(kEs8388DacControl25, steps);
}

bool beginModuleI2s(uint32_t sample_rate_hz) {
  if (i2s_active) {
    return true;
  }

  const int bck = M5.getPin(m5::pin_name_t::mbus_pin24);
  const int mck = M5.getPin(m5::pin_name_t::mbus_pin22);
  const int di = M5.getPin(m5::pin_name_t::mbus_pin26);
  const int ws = M5.getPin(m5::pin_name_t::mbus_pin21);
  const int data_out = M5.getPin(m5::pin_name_t::mbus_pin23);

  i2s_cfg.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX);
  i2s_cfg.sample_rate = sample_rate_hz;
  i2s_cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  i2s_cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  i2s_cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  i2s_cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  i2s_cfg.dma_buf_count = 8;
  i2s_cfg.dma_buf_len = 512;
  i2s_cfg.use_apll = false;
  i2s_cfg.tx_desc_auto_clear = true;
  i2s_cfg.fixed_mclk = 0;

  i2s_pin_config_t pin_cfg = {};
  pin_cfg.mck_io_num = mck;
  pin_cfg.bck_io_num = bck;
  pin_cfg.ws_io_num = ws;
  pin_cfg.data_out_num = data_out;
  pin_cfg.data_in_num = di;

  if (i2s_driver_install(kI2sPort, &i2s_cfg, 0, nullptr) != ESP_OK) {
    return false;
  }
  if (i2s_set_pin(kI2sPort, &pin_cfg) != ESP_OK) {
    i2s_driver_uninstall(kI2sPort);
    return false;
  }
  if (i2s_set_clk(kI2sPort, sample_rate_hz, I2S_BITS_PER_SAMPLE_16BIT,
                I2S_CHANNEL_STEREO) != ESP_OK) {
    i2s_driver_uninstall(kI2sPort);
    return false;
  }

  i2s_zero_dma_buffer(kI2sPort);
  i2s_start(kI2sPort);
  i2s_active = true;
  return true;
}

void freeJob(PlayJob* job) {
  if (job == nullptr) {
    return;
  }
  free(job->pcm);
  job->pcm = nullptr;
  job->pcm_len = 0;
}

void drainPlayQueue() {
  if (play_queue == nullptr) {
    return;
  }

  PlayJob dropped;
  while (xQueueReceive(play_queue, &dropped, 0) == pdTRUE) {
    freeJob(&dropped);
  }
}

void playWorker(void* /*arg*/) {
  for (;;) {
    PlayJob job;
    if (xQueueReceive(play_queue, &job, portMAX_DELAY) != pdTRUE) {
      continue;
    }

    PlayJob newer;
    while (xQueueReceive(play_queue, &newer, 0) == pdTRUE) {
      freeJob(&job);
      job = newer;
    }

    if (abort_playback) {
      freeJob(&job);
      continue;
    }

    setEs8388SampleRate(job.sample_rate_hz);
    setEs8388Mute(false);
    if (!abort_playback) {
      size_t bytes_written = 0;
      i2s_write(kI2sPort, job.pcm, job.pcm_len, &bytes_written, portMAX_DELAY);
    }
    setEs8388Mute(true);
    freeJob(&job);
  }
}

void ensurePlayWorker() {
  if (play_queue == nullptr) {
    play_queue = xQueueCreate(1, sizeof(PlayJob));
  }
  if (play_queue == nullptr) {
    return;
  }
  if (play_task == nullptr) {
    xTaskCreate(playWorker, "ext_audio", 8192, nullptr, 5, &play_task);
  }
}

}  // namespace

bool stackModuleAudioBegin() {
  if (codec_ready) {
    return true;
  }

  if (!initEs8388()) {
    return false;
  }

  writeReg8(kStm32Addr, kHpModeReg, kHpModeNational);
  writeEs8388Reg(kEs8388DacPower, kDacOutputOut1);
  setEs8388SampleRate(44100);
  setEs8388Volume(50);
  setEs8388Mute(true);
  ensurePlayWorker();
  codec_ready = true;
  return true;
}

bool stackModuleAudioReady() { return codec_ready; }

bool stackModuleEnsureI2s() {
  if (!codec_ready) {
    return false;
  }
  return beginModuleI2s(44100);
}

void stackModuleEndI2s() {
  if (!i2s_active) {
    return;
  }
  i2s_stop(kI2sPort);
  i2s_driver_uninstall(kI2sPort);
  i2s_active = false;
}

bool stackModuleIsI2sActive() { return i2s_active; }

void stackModulePreparePlayback() {
  if (!codec_ready) {
    return;
  }

  writeReg8(kStm32Addr, kHpModeReg, kHpModeNational);
  writeEs8388Reg(kEs8388DacPower, kDacOutputOut1);
  setEs8388SampleRate(44100);
}

void stackModuleSetVolume(uint8_t volume) {
  if (!codec_ready) {
    return;
  }
  setEs8388Volume(volume);
}

void stackModuleStop() {
  abort_playback = true;
  setEs8388Mute(true);
  drainPlayQueue();
  abort_playback = false;
}

bool stackModulePlayPcmAsync(uint8_t* pcm, size_t len, uint32_t sample_rate_hz) {
  if (!codec_ready || pcm == nullptr || len == 0) {
    free(pcm);
    return false;
  }

  ensurePlayWorker();
  if (play_queue == nullptr) {
    free(pcm);
    return false;
  }

  PlayJob job;
  job.pcm = pcm;
  job.pcm_len = len;
  job.sample_rate_hz = sample_rate_hz;

  PlayJob dropped;
  if (xQueueReceive(play_queue, &dropped, 0) == pdTRUE) {
    freeJob(&dropped);
  }

  if (xQueueSend(play_queue, &job, 0) != pdTRUE) {
    freeJob(&job);
    return false;
  }

  return true;
}
