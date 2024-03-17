/**
 * @file l293d.c
 * @author ottojo
 * @date 3/16/24
 * Description here TODO
 */
#include <sys/errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/motor.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(l293d, LOG_LEVEL_DBG);

#define DT_DRV_COMPAT l293d

struct l293d_config {
	const struct gpio_dt_spec en1_gpio;
	const struct gpio_dt_spec en2_gpio;
	const struct gpio_dt_spec in1_gpio;
	const struct gpio_dt_spec in2_gpio;
	const struct gpio_dt_spec in3_gpio;
	const struct gpio_dt_spec in4_gpio;
	const struct pinctrl_dev_config *const pcfg;
};

struct motor_config {
	const struct device *parent_l293d;
	uint8_t id;
	bool reversed;
};

struct motor_data {
	bool direction;
};

int l293d_enable_motor(const struct device *dev) {
	const struct motor_config *cfg = dev->config;
	const struct device *l293d_dev = cfg->parent_l293d;
	const struct l293d_config *l293d_cfg = l293d_dev->config;

	int err;
	if (cfg->id == 0) {
		err = gpio_pin_set_dt(&l293d_cfg->en1_gpio, 1);
	} else {
		err = gpio_pin_set_dt(&l293d_cfg->en2_gpio, 1);
	}
	if (err != 0) {
		LOG_ERR("%s: Error setting enable GPIO: %d", dev->name, err);
		return err;
	}

	return 0;
}

int l293d_disable_motor(const struct device *dev) {
	const struct motor_config *cfg = dev->config;
	const struct device *l293d_dev = cfg->parent_l293d;
	const struct l293d_config *l293d_cfg = l293d_dev->config;

	int err;
	if (cfg->id == 0) {
		err = gpio_pin_set_dt(&l293d_cfg->en1_gpio, 0);
	} else {
		err = gpio_pin_set_dt(&l293d_cfg->en2_gpio, 0);
	}
	if (err != 0) {
		LOG_ERR("%s: Error setting enable GPIO: %d", dev->name, err);
		return err;
	}

	return 0;
}

int l293d_set_motor_speed(const struct device *dev, uint8_t speed) {
	const struct motor_config *cfg = dev->config;
	const struct motor_data *data = dev->data;
	const struct device *l293d_dev = cfg->parent_l293d;
	const struct l293d_config *l293d_cfg = l293d_dev->config;

	const struct gpio_dt_spec *in1;
	const struct gpio_dt_spec *in2;

	if (cfg->id == 0) {
		in1 = &l293d_cfg->in1_gpio;
		in2 = &l293d_cfg->in2_gpio;
	} else {
		in1 = &l293d_cfg->in3_gpio;
		in2 = &l293d_cfg->in4_gpio;
	}

	int in1_val = 0;
	int in2_val = 0;
	if (speed > 0) {
		if (data->direction ^ cfg->reversed) {
			in1_val = 1;
			in2_val = 0;
		} else {
			in1_val = 0;
			in2_val = 1;
		}
	}

	int err1;
	int err2;

	err1 = gpio_pin_set_dt(in1, in1_val);
	err2 = gpio_pin_set_dt(in2, in2_val);

	if ((err1 != 0) || (err2 != 0)) {
		LOG_ERR("%s: Error setting direction GPIOs: %d, %d", dev->name, err1, err2);
		return -EIO;
	}

	return 0;
}

int l293d_set_motor_direction(const struct device *dev, bool direction) {
	const struct motor_config *cfg = dev->config;
	struct motor_data *data = dev->data;
	const struct device *l293d_dev = cfg->parent_l293d;
	const struct l293d_config *l293d_cfg = l293d_dev->config;

	const struct gpio_dt_spec *in1;
	const struct gpio_dt_spec *in2;

	if (cfg->id == 0) {
		in1 = &l293d_cfg->in1_gpio;
		in2 = &l293d_cfg->in2_gpio;
	} else {
		in1 = &l293d_cfg->in3_gpio;
		in2 = &l293d_cfg->in4_gpio;
	}

	bool actual_direction = direction;
	if (cfg->reversed) { actual_direction = !direction; }

	int err1;
	int err2;
	if (actual_direction) {
		err1 = gpio_pin_set_dt(in1, 1);
		err2 = gpio_pin_set_dt(in2, 0);
	} else {
		err1 = gpio_pin_set_dt(in1, 0);
		err2 = gpio_pin_set_dt(in2, 1);
	}
	if ((err1 != 0) || (err2 != 0)) {
		LOG_ERR("%s: Error setting direction GPIOs: %d, %d", dev->name, err1, err2);
		return -EIO;
	}

	data->direction = direction;
	return 0;
}

static struct motor_api l293d_motor_api = {.enable = &l293d_enable_motor,
										   .disable = &l293d_disable_motor,
										   .set_speed = &l293d_set_motor_speed,
										   .set_direction = &l293d_set_motor_direction};

static int l293d_init(const struct device *dev) {
	const struct l293d_config *cfg = dev->config;
	int err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		LOG_ERR("%s: Error initializing pinctrl", dev->name);
		return err;
	}
	LOG_INF("%s: Initialization complete", dev->name);
	return 0;
}

static int l293d_configure_motor(const struct device *dev, uint8_t id) {
	const struct l293d_config *cfg = dev->config;
	int err;

	if (id == 0) {
		err = gpio_pin_configure_dt(&cfg->en1_gpio, GPIO_OUTPUT_INACTIVE);
		if (err != 0) {
			LOG_ERR("%s: failed to initialize GPIO for enable1", dev->name);
			return err;
		}
		err = gpio_pin_configure_dt(&cfg->in1_gpio, GPIO_OUTPUT_INACTIVE);
		if (err != 0) {
			LOG_ERR("%s: failed to initialize GPIO for in1", dev->name);
			return err;
		}
		err = gpio_pin_configure_dt(&cfg->in2_gpio, GPIO_OUTPUT_INACTIVE);
		if (err != 0) {
			LOG_ERR("%s: failed to initialize GPIO for in2", dev->name);
			return err;
		}
	} else if (id == 1) {
		err = gpio_pin_configure_dt(&cfg->en2_gpio, GPIO_OUTPUT_INACTIVE);
		if (err != 0) {
			LOG_ERR("%s: failed to initialize GPIO for enable2", dev->name);
			return err;
		}
		err = gpio_pin_configure_dt(&cfg->in3_gpio, GPIO_OUTPUT_INACTIVE);
		if (err != 0) {
			LOG_ERR("%s: failed to initialize GPIO for in3", dev->name);
			return err;
		}
		err = gpio_pin_configure_dt(&cfg->in4_gpio, GPIO_OUTPUT_INACTIVE);
		if (err != 0) {
			LOG_ERR("%s: failed to initialize GPIO for in4", dev->name);
			return err;
		}
	} else {
		LOG_ERR("Invalid motor %d, valid values are 0 and 1", id);
		return -EINVAL;
	}
	LOG_ERR("%s: Initialized GPIOs for motor %d", dev->name, id);
	return 0;
}

static int l293d_motor_init(const struct device *dev) {
	const struct motor_config *cfg = dev->config;
	const struct device *l293d_dev = cfg->parent_l293d;

	int err;

	err = l293d_configure_motor(l293d_dev, cfg->id);
	if (err != 0) {
		LOG_ERR("%s: Error configuring GPIOs at %s", dev->name, l293d_dev->name);
		return err;
	}

	err = motor_disable(dev);
	if (err != 0) {
		LOG_ERR("%s: Error initially disabling motor", dev->name);
		return err;
	}
	err = motor_set_speed(dev, 0);
	if (err != 0) {
		LOG_ERR("%s: Error initially stopping motor", dev->name);
		return err;
	}
	err = motor_set_direction(dev, true);
	if (err != 0) {
		LOG_ERR("%s: Error setting initial direction", dev->name);
		return err;
	}

	LOG_INF("%s: Initialization complete", dev->name);

	return 0;
}

#define L293D_CHILD_INIT(node)                                                                                         \
	static const struct motor_config l293d_motor_##node##_config = {.parent_l293d = DEVICE_DT_GET(DT_PARENT(node)),    \
																	.id = DT_REG_ADDR(node),                           \
																	.reversed = DT_PROP(node, reverse)};               \
	static struct motor_data l293d_motor_##node##_data = {.direction = true};                                          \
	DEVICE_DT_DEFINE(node, l293d_motor_init, /* pm = */ NULL, /* data = */ &l293d_motor_##node##_data,                 \
					 /* config = */ &l293d_motor_##node##_config, POST_KERNEL, 51, &l293d_motor_api);

#define L293D_DEFINE(inst)                                                                                             \
	PINCTRL_DT_INST_DEFINE(inst);                                                                                      \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, L293D_CHILD_INIT);                                                         \
                                                                                                                       \
	static const struct l293d_config l293d_config_##inst = {                                                           \
			.en1_gpio = GPIO_DT_SPEC_INST_GET(inst, enable1_gpios),                                                    \
			.en2_gpio = GPIO_DT_SPEC_INST_GET(inst, enable2_gpios),                                                    \
			.in1_gpio = GPIO_DT_SPEC_INST_GET(inst, in1_gpios),                                                        \
			.in2_gpio = GPIO_DT_SPEC_INST_GET(inst, in2_gpios),                                                        \
			.in3_gpio = GPIO_DT_SPEC_INST_GET(inst, in3_gpios),                                                        \
			.in4_gpio = GPIO_DT_SPEC_INST_GET(inst, in4_gpios),                                                        \
			.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                                              \
                                                                                                                       \
	};                                                                                                                 \
	DEVICE_DT_INST_DEFINE(inst, l293d_init, /* pm = */ NULL, /* data = */ NULL, &l293d_config_##inst,                  \
						  /* level = */ POST_KERNEL, 50, /* api = */ NULL);

DT_INST_FOREACH_STATUS_OKAY(L293D_DEFINE)
