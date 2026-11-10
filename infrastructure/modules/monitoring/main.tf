# Cost monitoring for both clouds. Budgets alert — they do NOT cap spend.

resource "aws_budgets_budget" "monthly" {
  name         = "scapesense-aws-monthly"
  budget_type  = "COST"
  limit_amount = tostring(var.aws_monthly_budget_usd)
  limit_unit   = "USD"
  time_unit    = "MONTHLY"

  notification {
    comparison_operator        = "GREATER_THAN"
    threshold                  = 80
    threshold_type             = "PERCENTAGE"
    notification_type          = "ACTUAL"
    subscriber_email_addresses = var.alert_emails
  }

  notification {
    comparison_operator        = "GREATER_THAN"
    threshold                  = 100
    threshold_type             = "PERCENTAGE"
    notification_type          = "FORECASTED"
    subscriber_email_addresses = var.alert_emails
  }
}

resource "google_monitoring_notification_channel" "billing_email" {
  for_each     = toset(var.alert_emails)
  project      = var.project_id
  display_name = "ScapeSense billing alert (${each.value})"
  type         = "email"
  labels       = { email_address = each.value }
}

resource "google_billing_budget" "firebase" {
  billing_account = var.billing_account_id
  display_name    = "scapesense-gcp-monthly"

  amount {
    specified_amount {
      currency_code = "GBP"
      units         = tostring(var.gcp_monthly_budget_gbp)
    }
  }

  budget_filter {
    projects = ["projects/${var.project_number}"]
  }

  threshold_rules { threshold_percent = 0.5 }
  threshold_rules { threshold_percent = 0.9 }
  threshold_rules {
    threshold_percent = 1.0
    spend_basis       = "FORECASTED_SPEND"
  }

  all_updates_rule {
    monitoring_notification_channels = [
      for channel in google_monitoring_notification_channel.billing_email : channel.id
    ]
    disable_default_iam_recipients = false
  }
}
