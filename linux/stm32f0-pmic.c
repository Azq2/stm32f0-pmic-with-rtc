#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/power_supply.h>

#define PMIC_DCIN_GOOD				(1 << 0)
#define PMIC_DCIN_PRESENT			(1 << 1)
#define PMIC_BAT_PRESENT			(1 << 2)
#define PMIC_POWER_ON				(1 << 3)
#define PMIC_USER_POWER_OFF			(1 << 4)
#define PMIC_BAT_CHARGING			(1 << 5)
#define PMIC_BAT_CHARGE_EN			(1 << 6)
#define PMIC_BAT_LOW_TEMP			(1 << 7)
#define PMIC_BAT_HIGH_TEMP			(1 << 8)
#define PMIC_BAT_CHARGE_LOW_TEMP	(1 << 9)
#define PMIC_BAT_CHARGE_HIGH_TEMP	(1 << 10)
#define PMIC_PWR_KEY_PRESSED		(1 << 11)

#define PMIC_REG_STATUS					0
#define PMIC_REG_IRQ_STATUS				1
#define PMIC_REG_BAT_VOLTAGE			2
#define PMIC_REG_BAT_TEMP				3
#define PMIC_REG_BAT_MIN_TEMP			4
#define PMIC_REG_BAT_MAX_TEMP			5
#define PMIC_REG_BAT_PCT				6
#define PMIC_REG_DCIN_VOLTAGE			7
#define PMIC_REG_CPU_TEMP				8
#define PMIC_REG_GET_MAX_BAT_VOLTAGE	9
#define PMIC_REG_GET_MIN_BAT_VOLTAGE	10
#define PMIC_REG_POWER_OFF				11
#define PMIC_REG_RTC_TIME				12

struct stm32f0_pmic {
	struct device *dev;
	struct i2c_client *client;
	struct mutex xfer_lock;
	
	struct power_supply *psy_dcin;
	struct power_supply *psy_bat;
};

static uint32_t stm32f0_pmic_read(struct stm32f0_pmic *pmic, u8 reg) {
	u32 data;
	s32 ret;
	mutex_lock(&pmic->xfer_lock);
	ret = i2c_smbus_read_i2c_block_data(pmic->client, reg, 4, (u8 *) &data);
	mutex_unlock(&pmic->xfer_lock);
	// printk("stm32f0_pmic_read(%02X) = %08X\n", reg, le32_to_cpu(data));
	return ret != 4 ? 0 : le32_to_cpu(data);
}

static enum power_supply_property stm32f0_pmic_charger_prop[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_TEMP
};

static enum power_supply_property stm32f0_pmic_battery_prop[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
};

static char *battery_supplied_to[] = {
	"bat",
};

static int stm32f0_pmic_charger_get_property(struct power_supply *psy, enum power_supply_property psp, union power_supply_propval *val) {
	struct stm32f0_pmic *pmic = dev_get_drvdata(psy->dev.parent);
	switch (psp) {
		case POWER_SUPPLY_PROP_ONLINE:
			val->intval = (stm32f0_pmic_read(pmic, PMIC_REG_STATUS) & PMIC_DCIN_GOOD) ? 1 : 0;
		break;
		
		case POWER_SUPPLY_PROP_PRESENT:
			val->intval = (stm32f0_pmic_read(pmic, PMIC_REG_STATUS) & PMIC_DCIN_PRESENT) ? 1 : 0;
		break;
		
		case POWER_SUPPLY_PROP_VOLTAGE_NOW:
			val->intval = (s32) stm32f0_pmic_read(pmic, PMIC_REG_DCIN_VOLTAGE) * 1000;
		break;
		
		case POWER_SUPPLY_PROP_TEMP:
			val->intval = (s32) stm32f0_pmic_read(pmic, PMIC_REG_CPU_TEMP) / 100;
		break;
		
		default:
			/* Invalid property */
			return -EINVAL;
		break;
	}
	return 0;
}

static int stm32f0_pmic_battery_get_property(struct power_supply *psy, enum power_supply_property psp, union power_supply_propval *val) {
	struct stm32f0_pmic *pmic = dev_get_drvdata(psy->dev.parent);
	u32 tmp;
	switch (psp) {
		case POWER_SUPPLY_PROP_ONLINE:
			val->intval = (stm32f0_pmic_read(pmic, PMIC_REG_STATUS) & PMIC_DCIN_GOOD) ? 0 : 1;
		break;
		
		case POWER_SUPPLY_PROP_PRESENT:
			val->intval = (stm32f0_pmic_read(pmic, PMIC_REG_STATUS) & PMIC_BAT_PRESENT) ? 1 : 0;
		break;
		
		case POWER_SUPPLY_PROP_STATUS:
			tmp = stm32f0_pmic_read(pmic, PMIC_REG_STATUS);
			
			if ((tmp & PMIC_DCIN_GOOD)) {
				if ((tmp & PMIC_BAT_CHARGE_EN)) {
					val->intval = (tmp & PMIC_BAT_CHARGING) ? POWER_SUPPLY_STATUS_CHARGING : POWER_SUPPLY_STATUS_FULL;
				} else {
					val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
				}
			} else {
				val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
			}
		break;
		
		case POWER_SUPPLY_PROP_HEALTH:
			tmp = stm32f0_pmic_read(pmic, PMIC_REG_STATUS);
			
			if ((tmp & (PMIC_BAT_HIGH_TEMP | PMIC_BAT_CHARGE_HIGH_TEMP))) {
				val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;
			} else if ((tmp & (PMIC_BAT_LOW_TEMP | PMIC_BAT_CHARGE_LOW_TEMP))) {
				val->intval = POWER_SUPPLY_HEALTH_COLD;
			} else {
				val->intval = POWER_SUPPLY_HEALTH_GOOD;
			}
		break;
		
		case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
			val->intval = (s32) stm32f0_pmic_read(pmic, PMIC_REG_GET_MAX_BAT_VOLTAGE) * 1000;
		break;
		
		case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
			val->intval = (s32) stm32f0_pmic_read(pmic, PMIC_REG_GET_MIN_BAT_VOLTAGE) * 1000;
		break;
		
		case POWER_SUPPLY_PROP_VOLTAGE_NOW:
			val->intval = (s32) stm32f0_pmic_read(pmic, PMIC_REG_BAT_VOLTAGE) * 1000;
		break;
		
		case POWER_SUPPLY_PROP_CAPACITY:
			val->intval = (s32) stm32f0_pmic_read(pmic, PMIC_REG_BAT_PCT) / 1000;
		break;
		
		case POWER_SUPPLY_PROP_TEMP:
			val->intval = (s32) stm32f0_pmic_read(pmic, PMIC_REG_BAT_TEMP) / 100;
		break;
		
		case POWER_SUPPLY_PROP_TECHNOLOGY:
			val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
		
		default:
			/* Invalid property */
			return -EINVAL;
		break;
	}
	return 0;
}

static const struct power_supply_desc stm32f0_pmic_dcin_desc = {
	.name			= "dcin",
	.type			= POWER_SUPPLY_TYPE_MAINS,
	.properties		= stm32f0_pmic_charger_prop,
	.num_properties	= ARRAY_SIZE(stm32f0_pmic_charger_prop),
	.get_property	= stm32f0_pmic_charger_get_property,
};

static const struct power_supply_desc stm32f0_pmic_bat_desc = {
	.name					= "bat",
	.type					= POWER_SUPPLY_TYPE_BATTERY,
	.properties				= stm32f0_pmic_battery_prop,
	.num_properties			= ARRAY_SIZE(stm32f0_pmic_battery_prop),
	.get_property			= stm32f0_pmic_battery_get_property
};

static int stm32f0_pmic_register_psy(struct stm32f0_pmic *pmic) {
	struct power_supply_config psy_cfg = {};
	psy_cfg.supplied_to = battery_supplied_to;
	psy_cfg.num_supplicants = ARRAY_SIZE(battery_supplied_to);

	pmic->psy_dcin = power_supply_register(pmic->dev, &stm32f0_pmic_dcin_desc, &psy_cfg);
	if (IS_ERR(pmic->psy_dcin))
		goto err_psy_dcin;
	
	pmic->psy_bat = power_supply_register(pmic->dev, &stm32f0_pmic_bat_desc, NULL);
	if (IS_ERR(pmic->psy_bat))
		goto err_psy_bat;
	
	return 0;

err_psy_bat:
	power_supply_unregister(pmic->psy_dcin);
err_psy_dcin:
	return -EPERM;
}

static void stm32f0_pmic_unregister_psy(struct stm32f0_pmic *pmic) {
	power_supply_unregister(pmic->psy_dcin);
	power_supply_unregister(pmic->psy_bat);
}

static int stm32f0_pmic_probe(struct i2c_client *i2c_client) {
	struct stm32f0_pmic *pmic;
	int ret;
	
	pmic = devm_kzalloc(&i2c_client->dev, sizeof(*pmic), GFP_KERNEL);
	if (!pmic)
		return -ENOMEM;
	
	pmic->client = i2c_client;
	pmic->dev = &i2c_client->dev;
	i2c_set_clientdata(i2c_client, pmic);
	
	mutex_init(&pmic->xfer_lock);
	
	/*
	ret = stm32f0_pmic_init_device(pmic);
	if (ret) {
		dev_err(pmic->dev, "i2c communication err: %d", ret);
		return ret;
	}
	*/
	
	ret = stm32f0_pmic_register_psy(pmic);
	if (ret) {
		dev_err(pmic->dev, "power supplies register err: %d", ret);
		return ret;
	}
	/*
	ret = stm32f0_pmic_setup_irq(pmic);
	if (ret) {
		dev_err(pmic->dev, "irq handler err: %d", ret);
		stm32f0_pmic_unregister_psy(pmic);
		return ret;
	}
	*/
	return 0;
}

static int stm32f0_pmic_remove(struct i2c_client *cl) {
	struct stm32f0_pmic *pmic = i2c_get_clientdata(cl);

	//stm32f0_pmic_release_irq(pmic);
	stm32f0_pmic_unregister_psy(pmic);
	
	return 0;
}

static const struct of_device_id stm32f0_pmic_dt_ids[] = {
	{ .compatible = "zhumarin,stm32f0-pmic", },
	{ }
};
MODULE_DEVICE_TABLE(of, stm32f0_pmic_dt_ids);

static const struct i2c_device_id stm32f0_pmic_ids[] = {
	{"stm32f0-pmic", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, stm32f0_pmic_ids);

static struct i2c_driver stm32f0_pmic_driver = {
	.driver = {
		.name = "stm32f0-pmic",
		.of_match_table = of_match_ptr(pmic_dt_ids),
	},
	.probe_new = stm32f0_pmic_probe,
	.remove = stm32f0_pmic_remove,
	.id_table = stm32f0_pmic_ids,
};
module_i2c_driver(stm32f0_pmic_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kirill Zhumarin <kirill.zhumarin@gmail.com>");
MODULE_DESCRIPTION("PMIC with RTC based on STM32F0");
MODULE_INFO(intree, "Y");
