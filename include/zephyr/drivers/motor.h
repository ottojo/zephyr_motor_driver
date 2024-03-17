#ifndef ZEPHYR_MOTOR_DRIVER_MOTOR_H
#define ZEPHYR_MOTOR_DRIVER_MOTOR_H

#include <zephyr/device.h>

/**
 * Enable motor outputs, potentially causing power to be applied to the motor or enabling braking.
 * On motor drivers that don't support speed control, this shall not cause the motor to spin until a speed >0 has been commanded.
 * This shall not cause the motor to spin, even if motor_set_speed has been called with a nonzero value before.
 *
 * @return Negative error code for driver specific failure conditions
 */
typedef int (*motor_enable_function)(const struct device *device);

/**
 * Disconnect motor outputs.
 * This function should not cause active braking.
 * If disconnecting the motor without active braking is not possible, the driver must engage braking, equivalent to motor_set_speed(0).
 *
 * @return 0 on success
 * @return Negative error code for driver specific failure conditions
 */
typedef int (*motor_disable_function)(const struct device *device);

/**
 * Set motor speed. Has no effect while motor is disabled.
 * If motor does not support speed control, any nonzero speed value should cause the motor to spin.
 * A speed value of 0 must cause the motor to stop spinning as quickly as possible, enabling active braking if applicable.
 *
 * @return 0 on success
 * @return Negative error code for driver specific failure conditions
 */
typedef int (*motor_set_speed_function)(const struct device *device, uint8_t speed);

/**
 * Set motor direction.
 * @param direction true shall correspond to a "forwards" or "usual" rotation direction.
 */
typedef int (*motor_set_direction_function)(const struct device *device, bool direction);

struct motor_api {
	motor_enable_function enable;
	motor_disable_function disable;
	motor_set_speed_function set_speed;
	motor_set_direction_function set_direction;
};

static inline int motor_enable(const struct device *dev) {
	const struct motor_api *api = (const struct motor_api *) dev->api;

	return api->enable(dev);
}

static inline int motor_disable(const struct device *dev) {
	const struct motor_api *api = (const struct motor_api *) dev->api;

	return api->disable(dev);
}

static inline int motor_set_speed(const struct device *dev, uint8_t speed) {
	const struct motor_api *api = (const struct motor_api *) dev->api;

	return api->set_speed(dev, speed);
}

static inline int motor_set_direction(const struct device *dev, bool direction) {
	const struct motor_api *api = (const struct motor_api *) dev->api;

	return api->set_direction(dev, direction);
}

#endif//ZEPHYR_MOTOR_DRIVER_MOTOR_H
