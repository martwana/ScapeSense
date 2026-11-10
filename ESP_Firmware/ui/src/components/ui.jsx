import React, { useMemo, useState } from "react";
import { dailyAverageBps, dailyRecords, formatDayKey, formatWeightG, gasTone } from "../lib/format.js";
import bubbleIcon from "../../../../Logos/icon_03_bubble_counter.svg";
import cylinderIcon from "../../../../Logos/icon_01_co2_bottle_weight.svg";
import temperatureIcon from "../../../../Logos/icon_06_temperature.svg";

export function SystemIcon({ system, className = "h-8 w-8" }) {
	const src = system === "temperature" ? temperatureIcon : system === "cylinder" ? cylinderIcon : bubbleIcon;
	return <span aria-hidden='true' className={`system-icon ${className}`} style={{ maskImage: `url('${src}')`, WebkitMaskImage: `url('${src}')` }} />;
}

export function DisplayControls({ theme, setTheme, mode, setMode }) {
	return <div className='flex items-center gap-2'>
		<div role='group' aria-label='Colour theme' className='flex rounded-xl border border-white/10 bg-reef/80 p-1'>
			{['light', 'dark'].map((value) => <button key={value} type='button' aria-label={`${value === 'light' ? 'Light' : 'Dark'} theme`} title={`${value === 'light' ? 'Light' : 'Dark'} theme`} aria-pressed={theme === value} onClick={() => setTheme(value)} className={`flex h-9 w-9 items-center justify-center rounded-lg transition ${theme === value ? 'bg-aqua/15 text-aqua' : 'text-foam/50 hover:bg-white/5 hover:text-foam'}`}>
				<svg aria-hidden='true' viewBox='0 0 24 24' fill='none' stroke='currentColor' strokeWidth='1.6' strokeLinecap='round' strokeLinejoin='round' className='h-5 w-5'>
					{value === 'light' ? <><circle cx='12' cy='12' r='4' /><path d='M12 2v2m0 16v2M2 12h2m16 0h2M5 5l1.5 1.5m11 11L19 19M5 19l1.5-1.5m11-11L19 5' /></> : <path d='M20.5 14A8.7 8.7 0 0110 3.5 8.7 8.7 0 1020.5 14z' />}
				</svg>
			</button>)}
		</div>
		<button type='button' aria-label='Advanced view' aria-pressed={mode === 'advanced'} title={`Switch to ${mode === 'advanced' ? 'Basic' : 'Advanced'} view`} onClick={() => setMode(mode === 'advanced' ? 'basic' : 'advanced')} className={`flex h-11 items-center gap-2 rounded-xl border px-3 text-xs font-semibold transition ${mode === 'advanced' ? 'border-aqua/40 bg-aqua/10 text-aqua' : 'border-white/10 bg-reef/80 text-foam/65 hover:bg-white/5'}`}>
			<svg aria-hidden='true' viewBox='0 0 20 20' fill='none' stroke='currentColor' strokeWidth='1.5' strokeLinecap='round' className='h-4 w-4'><path d='M3 5h14M3 10h14M3 15h14' /><path d='M7 3v4m6 1v4m-6 1v4' /></svg>
			{mode === 'advanced' ? 'Advanced' : 'Basic'}
		</button>
	</div>;
}

export function Widget({ title, state, icon, children }) {
	return <section className='widget rounded-3xl p-5 backdrop-blur' data-tone={state.tone} data-alert={state.severity}>
		<div className='mb-5 flex items-center justify-between gap-3'>
			<h2 className='text-sm font-semibold text-foam/75'>{title}</h2>{icon}
		</div>
		{children}
		<p className='widget-status mt-5 text-xs font-semibold'>{state.label}</p>
	</section>;
}

export function Panel({ title, eyebrow, action, children, className = "" }) {
	return (
		<section
			className={`rounded-3xl border border-white/10 bg-reef/80 p-5 shadow-glow backdrop-blur ${className}`}
		>
			<div className='mb-4 flex flex-wrap items-start justify-between gap-4'>
				<div>
					{eyebrow && (
						<p className='text-xs font-semibold uppercase tracking-[0.24em] text-aqua/80'>
							{eyebrow}
						</p>
					)}
					<h2 className='mt-1 text-lg font-semibold text-white'>{title}</h2>
				</div>
				{action}
			</div>
			{children}
		</section>
	);
}

export function CollapsiblePanel({
	title,
	eyebrow,
	children,
	defaultOpen = false,
	className = "",
}) {
	const [open, setOpen] = useState(defaultOpen);
	return (
		<section
			className={`rounded-3xl border border-white/10 bg-reef/80 shadow-glow backdrop-blur ${className}`}
		>
			<button
				type='button'
				onClick={() => setOpen((value) => !value)}
				aria-expanded={open}
				className='flex w-full items-center justify-between gap-4 p-5 text-left'
			>
				<div>
					{eyebrow && (
						<p className='text-xs font-semibold uppercase tracking-[0.24em] text-aqua/80'>
							{eyebrow}
						</p>
					)}
					<h2 className='mt-1 text-lg font-semibold text-white'>{title}</h2>
				</div>
				<svg
					viewBox='0 0 20 20'
					fill='none'
					className={`h-5 w-5 shrink-0 text-foam/50 transition-transform duration-200 ${open ? "rotate-180" : ""}`}
				>
					<path
						d='M5 7.5l5 5 5-5'
						stroke='currentColor'
						strokeWidth='1.8'
						strokeLinecap='round'
						strokeLinejoin='round'
					/>
				</svg>
			</button>
			{open && <div className='px-5 pb-5'>{children}</div>}
		</section>
	);
}

export function Metric({ label, value, detail, icon, tone = "text-white" }) {
	return (
		<div className='rounded-2xl border border-white/10 bg-abyss/50 p-4 backdrop-blur'>
			<p className='text-xs font-medium uppercase tracking-[0.18em] text-foam/50'>
				{icon}
				{label}
			</p>
			<p className={`mt-2 text-3xl font-semibold tracking-tight ${tone}`}>
				{value}
			</p>
			{detail && <p className='mt-1 text-sm text-foam/55'>{detail}</p>}
		</div>
	);
}

export function InfoRow({ label, value }) {
	return (
		<div className='flex justify-between gap-4 border-b border-white/5 pb-2 last:border-0'>
			<span className='text-foam/50'>{label}</span>
			<span className='text-right font-medium capitalize'>{value}</span>
		</div>
	);
}

export function Field({ label, suffix, ...props }) {
	return (
		<label className='block'>
			<span className='mb-1.5 block text-xs font-medium uppercase tracking-[0.16em] text-foam/55'>
				{label}
			</span>
			<span className='relative block'>
				<input {...props} />
				{suffix && (
					<span className='pointer-events-none absolute right-3 top-1/2 -translate-y-1/2 text-sm text-foam/40'>
						{suffix}
					</span>
				)}
			</span>
		</label>
	);
}

export function Button({
	children,
	variant = "secondary",
	className = "",
	...props
}) {
	const styles = {
		primary: "bg-aqua text-abyss hover:bg-aqua/85",
		secondary: "border border-white/10 bg-white/5 text-foam hover:bg-white/10",
		danger: "border border-coral/30 bg-coral/10 text-coral hover:bg-coral/20",
		ghost: "text-aqua hover:bg-aqua/10",
	};
	return (
		<button
			className={`rounded-xl px-4 py-2 text-sm font-semibold transition ${styles[variant]} ${className}`}
			{...props}
		>
			{children}
		</button>
	);
}

export function StatusPill({ active, children, tone = "aqua" }) {
	const colors = {
		aqua: active
			? "border-aqua/40 bg-aqua/10 text-aqua"
			: "border-white/10 bg-white/5 text-foam/55",
		coral: active
			? "border-coral/40 bg-coral/10 text-coral"
			: "border-white/10 bg-white/5 text-foam/55",
		amber: active
			? "border-amber/40 bg-amber/10 text-amber"
			: "border-white/10 bg-white/5 text-foam/55",
	};
	return (
		<span
			className={`inline-flex items-center gap-1.5 rounded-full border px-3 py-1 text-xs font-semibold ${colors[tone]}`}
		>
			{children}
		</span>
	);
}

export function TabBar({ tabs, active, onChange }) {
	return (
		<nav
			aria-label='Sections'
			className='inline-flex rounded-2xl border border-white/10 bg-abyss/70 p-1 backdrop-blur'
		>
			{tabs.map((tab) => (
				<button
					key={tab.id}
					type='button'
					aria-current={active === tab.id ? "page" : undefined}
					onClick={() => onChange(tab.id)}
					className={`flex items-center gap-2 rounded-xl px-4 py-2 text-sm font-semibold transition ${
						active === tab.id
							? "bg-aqua text-abyss shadow"
							: "text-foam/60 hover:bg-white/5 hover:text-foam"
					}`}
				>
					{tab.icon}
					{tab.label}
				</button>
			))}
		</nav>
	);
}

export function GasGauge({ status }) {
	const percent = Math.max(0, Math.min(100, status.gasRemainingPercent || 0));
	return (
		<div>
			<div className='mb-3 flex items-end justify-between'>
				<div>
					<p className='text-sm text-foam/55'>Estimated CO₂ remaining</p>
					<p className='mt-1 text-4xl font-semibold text-white'>
						{status.gasConfigured ? `${percent.toFixed(1)}%` : "--"}
					</p>
				</div>
				<p className='text-right text-sm text-foam/60'>
					{status.gasConfigured
						? formatWeightG(status.gasRemainingG, true)
						: "Configure cylinder"}
				</p>
			</div>
			<div className='h-4 overflow-hidden rounded-full bg-white/10'>
				<div
					className={`h-full rounded-full transition-all duration-500 ${gasTone(percent)}`}
					style={{ width: `${status.gasConfigured ? percent : 0}%` }}
				/>
			</div>
			<div className='mt-2 flex justify-between text-xs text-foam/45'>
				<span>Empty</span>
				<span>Low below 20%</span>
				<span>Full</span>
			</div>
		</div>
	);
}

export function DailyChart({ stats }) {
	const records = useMemo(() => {
		return dailyRecords(stats);
	}, [stats]);
	const max = Math.max(1, ...records.map((record) => record.count || 0));

	return (
		<div className='flex h-48 items-end gap-2 overflow-x-auto' aria-label='Daily bubble totals'>
			{records.length === 0 && (
				<p className='text-sm text-foam/50'>No daily history yet.</p>
			)}
			{records.map((record, index) => (
				<div
					key={`${record.day}-${index}`}
					className='group flex min-w-[3rem] flex-1 flex-col items-center gap-2'
				>
					<span className='text-xs text-foam/70'>{(record.count || 0).toLocaleString()}</span>
					<div className='relative flex h-28 w-full items-end rounded-lg bg-white/5'>
						<div
							className={`w-full rounded-lg transition-all ${index === records.length - 1 ? "bg-aqua" : "bg-lagoon group-hover:bg-aqua/60"}`}
							style={{
								height: `${Math.max(3, ((record.count || 0) / max) * 100)}%`,
							}}
							title={`${record.count || 0} bubbles`}
						/>
					</div>
					<span className='text-[10px] text-foam/45'>
						{formatDayKey(record.day)}
					</span>
				</div>
			))}
		</div>
	);
}

export function DailyTable({ stats }) {
	const records = dailyRecords(stats).reverse();
	const time = (epoch) => epoch ? new Date(epoch * 1000).toLocaleTimeString('en-GB', { timeZone: 'Europe/London', hour: '2-digit', minute: '2-digit' }) : '--';
	return <div className='overflow-x-auto'>
		<table className='w-full whitespace-nowrap text-left text-sm'>
			<caption className='mb-3 text-left text-xs text-foam/55'>UK time. Average uses total bubbles divided by recorded flow time, excluding idle time. Legacy estimates are marked ~.</caption>
			<thead><tr>{['Date', 'Start time', 'End time', 'Total bubbles', 'Avg BPS'].map((label) => <th key={label} scope='col' className='px-3 py-2 text-foam/60'>{label}</th>)}</tr></thead>
			<tbody>{records.map((record) => {
				const average = dailyAverageBps(record);
				const estimated = record.flowDurationEstimated || !Number.isFinite(record.activeSeconds);
				return <tr key={record.day} className='border-t border-white/10'>
				<td className='px-3 py-3'>{formatDayKey(record.day)}/{String(record.day).slice(0, 4)}</td>
				<td className='px-3 py-3'>{time(record.start)}</td>
				<td className='px-3 py-3'>{record.active ? 'Flowing' : time(record.stop)}</td>
				<td className='px-3 py-3'>{(record.count || 0).toLocaleString()}</td>
				<td className='px-3 py-3' title={estimated ? 'Estimated from older start/stop data; separate flow sessions were not saved.' : 'Calculated from accumulated flow sessions.'}>
					{average === null ? '--' : `${estimated ? '~' : ''}${average.toFixed(3)}`}
				</td>
			</tr>;
			})}</tbody>
		</table>
		{!records.length && <p className='py-4 text-sm text-foam/50'>No daily history yet.</p>}
	</div>;
}

export function Toast({ notice }) {
	if (!notice) return null;
	const error = notice.startsWith("Error");
	return (
		<div
			className={`fixed bottom-5 right-5 z-50 rounded-2xl border px-4 py-3 text-sm shadow-2xl ${error ? "border-coral/40 bg-coral/15 text-coral" : "border-aqua/40 bg-abyss text-foam"}`}
		>
			{notice}
		</div>
	);
}
