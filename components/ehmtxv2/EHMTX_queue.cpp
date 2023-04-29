#include "esphome.h"

namespace esphome
{

  EHMTX_queue::EHMTX_queue(EHMTX *config)
  {
    this->config_ = config;
    this->endtime = 0;
    this->last_time = 0;
    this->centerx_ = 0;
    this->shiftx_ = 0;
    this->screen_time = 0;
    this->mode = MODE_EMPTY;
    this->icon_name = "";
    this->icon = 0;
    this->text = "";
    this->default_font = true;
  }

  void EHMTX_queue::status()
  {
    switch (this->mode)
    {
    case MODE_EMPTY:
      ESP_LOGD(TAG, "queue: empty slot");
      break;
    case MODE_BLANK:
      ESP_LOGD(TAG, "queue: blank screen for %d sec", this->screen_time);
      break;
    case MODE_CLOCK:
      ESP_LOGD(TAG, "queue: clock for %d sec", this->screen_time);
      break;
    case MODE_DATE:
      ESP_LOGD(TAG, "queue: date for %d sec", this->screen_time);
      break;
    case MODE_FULL_SCREEN:
      ESP_LOGD(TAG, "queue: full screen: %s for %d sec", this->icon_name.c_str(), this->screen_time);
      break;
    case MODE_ICONSCREEN:
      ESP_LOGD(TAG, "queue: icon screen: %s text: %s for %d sec", this->icon_name.c_str(), this->text.c_str(), this->screen_time);
      break;
    case MODE_TEXT:
      ESP_LOGD(TAG, "queue: text text: %s for %d sec", this->text.c_str(), this->screen_time);
      break;
    case MODE_RAINBOW_ICON:
      ESP_LOGD(TAG, "queue: rainbow icon: %s text: %s for %d sec", this->icon_name.c_str(), this->text.c_str(), this->screen_time);
      break;
    case MODE_RAINBOW_TEXT:
      ESP_LOGD(TAG, "queue: rainbow text: %s for %d sec", this->text.c_str(), this->screen_time);
      break;
    case MODE_RAINBOW_CLOCK:
      ESP_LOGD(TAG, "queue: clock for %d sec", this->screen_time);
      break;
    default:
      ESP_LOGD(TAG, "queue: UPPS");
      break;
    }
  }

  void EHMTX_queue::update_screen()
  {
    if (millis() - this->config_->last_rainbow_time >= this->config_->rainbow_interval)
    {
      this->config_->hue_++;
      if (this->config_->hue_ == 360)
      {
        this->config_->hue_ = 0;
      }
      float red, green, blue;
      esphome::hsv_to_rgb(this->config_->hue_, 0.8, 0.8, red, green, blue);
      this->config_->rainbow_color = Color(uint8_t(255 * red), uint8_t(255 * green), uint8_t(255 * blue));
      this->config_->last_rainbow_time = millis();
    }

    if ((this->mode == MODE_ICONSCREEN) || (this->mode == MODE_RAINBOW_ICON))
    {
      if (millis() - this->config_->last_scroll_time >= this->config_->scroll_interval && this->pixels_ > TEXTSTARTOFFSET)
      {
        this->shiftx_++;
        if (this->shiftx_ > this->pixels_ + TEXTSTARTOFFSET)
        {
          this->shiftx_ = 0;
        }
        this->config_->last_scroll_time = millis();
      }
    }
    if ((this->mode == MODE_TEXT) || (this->mode == MODE_RAINBOW_TEXT))
    {
      if (millis() - this->config_->last_scroll_time >= this->config_->scroll_interval && this->pixels_ >= 32)
      {
        this->shiftx_++;
        if (this->shiftx_ > this->pixels_ + 32)
        {
          this->shiftx_ = 0;
        }
        this->config_->last_scroll_time = millis();
      }
    }

    if (millis() - this->config_->last_anim_time >= this->config_->icons[this->icon]->frame_duration)
    {
      this->config_->icons[this->icon]->next_frame();
      this->config_->last_anim_time = millis();
    }
  }

  void EHMTX_queue::draw()
  {
    display::Font *font = this->default_font ? this->config_->default_font : this->config_->special_font;
    int8_t yoffset = this->default_font ? this->config_->default_xoffset : this->config_->special_xoffset;
    int8_t xoffset = this->default_font ? this->config_->default_yoffset : this->config_->special_yoffset;
    int8_t extraoffset = 0;
    Color color_;
    if (this->config_->is_running)
    {
      switch (this->mode)
      {
      case MODE_EMPTY:
        break;
      case MODE_BLANK:
        break;
      case MODE_RAINBOW_CLOCK:
      case MODE_CLOCK:
        if (this->config_->clock->now().timestamp > 6000) // valid time
        {
          color_ = (this->mode == MODE_RAINBOW_CLOCK) ? this->config_->rainbow_color : this->text_color;
          time_t ts = this->config_->clock->now().timestamp;
          this->config_->display->strftime(xoffset + 15, yoffset, font, color_, display::TextAlign::BASELINE_CENTER, this->config_->time_fmt.c_str(),
                                           this->config_->clock->now());
          if ((this->config_->clock->now().second % 2 == 0) && this->config_->show_seconds)
          {
            this->config_->display->draw_pixel_at(0, 0, color_);
          }
          if (this->mode != MODE_RAINBOW_CLOCK){
            this->config_->draw_day_of_week();
          }
        }
        else
        {
          this->config_->display->print(15 + xoffset, yoffset, font, this->config_->alarm_color, display::TextAlign::BASELINE_CENTER, "!t!");
        }
        break;
      case MODE_DATE:
        if (this->config_->clock->now().timestamp > 6000) // valid time
        {
          time_t ts = this->config_->clock->now().timestamp;
          this->config_->display->strftime(xoffset + 15, yoffset, font, this->text_color, display::TextAlign::BASELINE_CENTER, this->config_->date_fmt.c_str(),
                                           this->config_->clock->now());
          if ((this->config_->clock->now().second % 2 == 0) && this->config_->show_seconds)
          {
            this->config_->display->draw_pixel_at(0, 0, this->config_->clock_color);
          }
          this->config_->draw_day_of_week();
        }
        else
        {
          this->config_->display->print(xoffset + 15, yoffset, font, this->config_->alarm_color, display::TextAlign::BASELINE_CENTER, "!d!");
        }
        break;
      case MODE_FULL_SCREEN:
        this->config_->display->image(0, 0, this->config_->icons[this->icon]);
        break;
      case MODE_ICONSCREEN:
      case MODE_RAINBOW_ICON:
      {
        if (this->pixels_ > TEXTSTARTOFFSET)
        {
          extraoffset = TEXTSTARTOFFSET;
        }
        if (this->config_->display_gauge)
        {
          extraoffset += 2;
        }

        color_ = (this->mode == MODE_RAINBOW_ICON) ? this->config_->rainbow_color : this->text_color;

        this->config_->display->print(this->centerx_ + TEXTSCROLLSTART - this->shiftx_ + extraoffset + xoffset, yoffset, font, color_, esphome::display::TextAlign::BASELINE_LEFT,
                                      this->text.c_str());
        if (this->config_->display_gauge)
        {
          this->config_->display->image(2, 0, this->config_->icons[this->icon]);
          this->config_->display->line(10, 0, 10, 7, esphome::display::COLOR_OFF);
        }
        else
        {
          this->config_->display->line(8, 0, 8, 7, esphome::display::COLOR_OFF);
          this->config_->display->image(0, 0, this->config_->icons[this->icon]);
        }
      }
      break;
      case MODE_TEXT:
      case MODE_RAINBOW_TEXT:

        if (this->pixels_ > 32)
        {
          extraoffset = 32;
        }
        if (this->config_->display_gauge)
        {
          extraoffset += 2;
        }
        color_ = (this->mode == MODE_RAINBOW_TEXT) ? this->config_->rainbow_color : this->text_color;
        this->config_->display->print(this->centerx_ - this->shiftx_ + xoffset + extraoffset, yoffset, font, color_, esphome::display::TextAlign::BASELINE_LEFT,
                                      this->text.c_str());
        break;
      default:
        break;
      }
      this->update_screen();
    } 
  }

  void EHMTX_queue::hold_slot(uint8_t _sec)
  {
    this->endtime += _sec;
    ESP_LOGD(TAG, "hold for %d secs", _sec);
  }

  // TODO void EHMTX_queue::set_mode_icon()
  void EHMTX_queue::set_text(std::string text, uint8_t icon, uint16_t pixel, uint16_t et, uint16_t screen_time)
  {
    this->text = text;
    this->pixels_ = pixel;

    if (pixel < 23)
    {
      this->centerx_ = ceil((22 - pixel) / 2);
    }

    this->shiftx_ = 0;
    float display_duration = ceil((this->config_->scroll_count * (TEXTSTARTOFFSET + pixel) * this->config_->scroll_interval) / 1000);
    this->screen_time = (display_duration > screen_time) ? display_duration : screen_time;
    ESP_LOGD(TAG, "display text: %s pixels %d calculated: %d screen_time: %d", text.c_str(), pixel, this->screen_time, screen_time);
    this->endtime = this->config_->clock->now().timestamp + et * 60;
    this->icon = icon;
  }
}
