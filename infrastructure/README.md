# ScapeSense Infrastructure

Terraform for both clouds: **AWS** (IoT Core, Lambda, S3, Jobs, budgets),
**GCP/Firebase** (project, Auth, Firestore, RTDB, Hosting, IAM, budget) and
**Cloudflare** (DNS for scapesense.io — the domain is registered at
Cloudflare, so the zone already exists and is authoritative). One environment
(`prod`) today; add `environments/dev` later by reusing the same modules
with a separate state key.

```text
infrastructure/
├── bootstrap/                 # creates the remote state bucket (run once, local state)
├── modules/
│   ├── cloudflare/            # existing scapesense.io zone lookup (records live in env)
│   ├── firebase/              # GCP project + Firebase attachment + APIs + web app
│   ├── firestore/             # (default) DB, expiresAt TTL, deny-all rules
│   ├── realtime-database/     # RTDB instance for live metrics
│   ├── authentication/        # Identity Platform, email/password only
│   ├── hosting/               # Hosting site + app.scapesense.io custom domain
│   ├── iam/                   # Functions runtime service account (least-privilege)
│   ├── iot-core/              # Thing Type/Group, per-device policy, IoT->CW logging
│   ├── iot-provisioning/      # Fleet Provisioning template + claim policy + role
│   ├── iot-jobs/              # OTA job template (presigned S3 manifest URL)
│   ├── lambda/                # IoT->Firebase bridge fn, topic rules, secret
│   ├── storage/               # private S3 bucket for signed firmware releases
│   └── monitoring/            # AWS Budgets + GCP billing budget alerts
└── environments/prod/         # wires modules together; S3 backend
```

## Prerequisites

| Tool | Install |
|---|---|
| Terraform >= 1.10 | `tfenv install` (reads `.terraform-version`) or `brew install terraform` |
| AWS credentials | `aws configure` or `aws sso login` — needs permission to create IAM/IoT/S3/Budgets/Lambda/SecretsManager resources |
| GCP credentials | `gcloud auth application-default login` + `gcloud auth application-default set-quota-project <project>` — needs a billing account ID (`gcloud billing accounts list`) |
| Cloudflare token | Scoped **Edit zone DNS** API token restricted to the `scapesense.io` zone (dash.cloudflare.com → My Profile → API Tokens). Put `cloudflare_api_token = "..."` in `terraform.tfvars` or export `CLOUDFLARE_API_TOKEN` |

No secrets live in this repo: `terraform.tfvars`, `backend.hcl`, `*.tfstate` and
credential files are gitignored.

## Runbook

### Step 0 — state backend (once)

```bash
cd infrastructure/bootstrap
terraform init && terraform apply   # local state — this stack has no backend
terraform output state_bucket_name  # -> scapesense-terraform-state-<acct>
cd ../environments/prod
cp backend.hcl.example backend.hcl  # fill in the bucket name
terraform init -backend-config=backend.hcl
```

### Step A — infrastructure

```bash
cp terraform.tfvars.example terraform.tfvars   # fill in IDs/emails/token
terraform apply                                # custom_domain_enabled=true
terraform output firebase_required_dns_updates # exact records Firebase wants
```

No registrar delegation step — the `scapesense.io` zone at Cloudflare is
already authoritative.

### Step B — create the required records in Cloudflare

Copy each record from `terraform output firebase_required_dns_updates` into
`app_dns_records` in `terraform.tfvars` and re-apply:

```hcl
app_dns_records = [
  { name = "app.scapesense.io", type = "TXT", rdata = "..." },
  { name = "app.scapesense.io", type = "A",   rdata = "..." },
]
```

(The values come from Firebase — the Cloudflare records can't be planned
until Firebase reports them, so this is deliberately a copy/paste step.
Records are created DNS-only/`proxied = false` so Firebase verification and
certificate issuance work.)

Firebase then finishes cert provisioning for `app.scapesense.io` (minutes).
Firebase Hosting manages the TLS certificate — no ACM.

### Step C — deploy Firebase application bits

Firebase CLI handles deployment actions Terraform doesn't own:

```bash
cd Firebase
firebase login
firebase use <project-id>            # or firebase use --add
firebase deploy --only functions,hosting,database
```

- Functions runtime SA = `terraform output functions_service_account`
  (pass `--service-account` or set in deploy config).
- RTDB rules (`database.rules.json`) still to be authored — clients may read
  only devices they own.
- Generate the Web Push **VAPID** key in the Firebase console (Cloud Messaging
  → Web Push certificates) and put only the *public* key in
  `Firebase/web/public/push-config.json`.

### Step D — dev device certificate (Phase 2)

Manufactured devices will mint certificates through Fleet Provisioning. For the
first connectivity proof, create one dev Thing + cert by hand:

```bash
aws iot create-thing --thing-name scapesense-dev-001 \
    --thing-type-name scapesense-device
aws iot create-keys-and-certificate --set-as-active \
    --certificate-pem-outfile dev-001.cert.pem \
    --public-key-outfile dev-001.public.key \
    --private-key-outfile dev-001.private.key   # keep off the repo
aws iot attach-policy --policy-name scapesense-device-policy \
    --target <certificate-arn>
aws iot attach-thing-principal --thing-name scapesense-dev-001 \
    --principal <certificate-arn>
terraform output iot_data_endpoint             # MQTT endpoint, port 8883

# Register the OTA job template once (no Terraform resource exists for it):
terraform output -raw create_job_template_command | sh
```

## Conventions

- **Devices are not Terraform resources.** Fleet Provisioning + Firestore track
  them; Terraform owns shared infra only.
- Regions: AWS `eu-west-2` (London); Firestore/Functions `europe-west2`; RTDB
  `europe-west1` (RTDB doesn't offer europe-west2).
- `google_project` uses `deletion_policy = "PREVENT"` and Firestore
  `delete_protection_state = "DELETE_PROTECTION_ENABLED"` — deliberate teardown
  requires disabling these first.
- Budgets alert; they do not cap spend.
- The Lambda bridge is a stub until Phase 3 wires the real Firebase
  integration credential into `scapesense/firebase-integration` (Secrets
  Manager — never in tfvars or source).
