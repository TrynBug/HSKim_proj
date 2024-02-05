using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;


public class Healthbar : MonoBehaviour
{
    [SerializeField]
    Canvas canvas;

    [SerializeField]
    Image fillImage;

    [SerializeField, Tooltip("Whether the healthbar should be hidden when health is empty")]
    bool hideEmpty = false;


    public float MaxHealth { get; set; } = 100.0f;
    public float CurrentHealth { get; set; } = 100.0f;


    [SerializeField, Min(0.1f), Tooltip("Controls how fast changes will be animated in points/second")]
    float changeSpeed = 500;

    float currentValue;


    private void Start()
    {
        currentValue = CurrentHealth;
    }

    private void Update()
    {
        currentValue = Mathf.MoveTowards(currentValue, CurrentHealth, Time.deltaTime * changeSpeed);

        UpdateFillbar();
    }


    void UpdateFillbar()
    {
        // Update the fill amount
        float value = Mathf.InverseLerp(0, MaxHealth, currentValue);

        fillImage.fillAmount = value;
    }




    public void SetHealth(float health)
    {
        if (CurrentHealth > health)
        {
            ApplyDamage(CurrentHealth - health);
        }
        else if (CurrentHealth < health)
        {
            AddHealth(health - CurrentHealth);
        }
    }

    public void AddHealth(float amount)
    {
        CurrentHealth += amount;
        CurrentHealth = Mathf.Clamp(CurrentHealth, 0, MaxHealth);
    }

    public void ApplyDamage(float damage)
    {
        CurrentHealth = Mathf.Clamp(CurrentHealth - damage, 0, MaxHealth);
    }
}

