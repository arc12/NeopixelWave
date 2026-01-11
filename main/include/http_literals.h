// Literals for HTTP responses. Allows the bulk text to live in flash (const char* declarations are stored in flash)
// Conventions:
//  end names witl "_l" so  it is easy to see where variables are defined
//  use r( and )r as the start/end delimiters

const char* style_l = R"r(
<style>
body {font-size: 5vw;}
h1 {font-size: 8vw;}
h2 {font-size: 5vw;}
td {font-size: 4vw;}
input {font-size: 4vw; margin-top: 1vw; margin-bottom:1vw;}
select {font-size: 4vw; margin-top: 1vw; margin-bottom:1vw;}
li {margin-top: 2vw;}
li.compact {margin-top: 0vw; font-size: 2vw;}
p {font-size: 2vw;}
</style>
)r";

;

// TODO consider adding a maxlength to input forms.

// includes a shitty bit of JS because HTML has a selected attribute on the option, not a value attribute on the selected element!!
const char* cmap_l =R"r(
<script>
document.addEventListener('DOMContentLoaded', () => {
var selectElement = document.getElementById('channel_map');
selectElement.value = '%c';
});
</script>
Channel Mapping:
<select name="channel_map" id="channel_map"><option value="r">RGB</option><option value="h">HSL</option></select>
<br/>
)r";

;

// includes a shitty bit of JS because HTML has a selected attribute on the option, not a value attribute on the selected element!!
const char* waveform_l =R"r(
<script>
document.addEventListener('DOMContentLoaded', () => {
var selectElement = document.getElementById('waveform_1');
selectElement.value = '%c';
selectElement = document.getElementById('waveform_2');
selectElement.value = '%c';
selectElement = document.getElementById('waveform_3');
selectElement.value = '%c';
});
</script>
Waveform and Ratio (0.0 - 1.0)
<table width="100%%">
<tr>
<td><select name="waveform_1" id="waveform_1"><option value="c">Cosine</option><option value="s">Square</option><option value="t">Triangle</option><option value="u">Uniform</option></select></td>
<td><select name="waveform_2" id="waveform_2"><option value="c">Cosine</option><option value="s">Square</option><option value="t">Triangle</option><option value="u">Uniform</option></select></td>
<td><select name="waveform_3" id="waveform_3"><option value="c">Cosine</option><option value="s">Square</option><option value="t">Triangle</option><option value="u">Uniform</option></select></td>
</tr>
<tr>
<td><input name="ratio_1" type="text" value="%.2f" size="5"/></td>
<td><input name="ratio_2" type="text" value="%.2f" size="5"/></td>
<td><input name="ratio_3" type="text" value="%.2f" size="5"/></td>
</tr>
</table>
)r";

;

const char* wavelength_l = R"r(
Wavelength and Phase (-1.0 - 1.0)
<table width="100%%">
<tr>
<td><input name="lambda_1" type="text" value="%.1f" size="5"/>p</td>
<td><input name="lambda_2" type="text" value="%.1f" size="5"/>p</td>
<td><input name="lambda_3" type="text" value="%.1f" size="5"/>p</td>
</tr>
<tr>
<td><input name="phase_1" type="text" value="%.2f" size="5"/></td>
<td><input name="phase_2" type="text" value="%.2f" size="5"/></td>
<td><input name="phase_3" type="text" value="%.2f" size="5"/></td>
</tr>
<table>
)r";

;

const char* amplitude_l = R"r(
Amplitude (0.0 - 1.0) - extra: <input type="checkbox" onclick="javascript:document.getElementById('am').style.display = this.checked?'block':'none';" style="width: 4vw; height: 4vw;"/>
<table width="100%%">
<tr>
<td><input name="amplitude_1" type="text" value="%.2f" size="5"/></td>
<td><input name="amplitude_2" type="text" value="%.2f" size="5"/></td>
<td><input name="amplitude_3" type="text" value="%.2f" size="5"/></td>
</tr>
</table>
)r";

;

const char* am_l = R"r(
<div id="am" style="display: none">
Modulation depth, period, ratio, offset
<table width="100%%">
<tr>
<td><input name="amp_mod_depth_1" type="text" value="%.2f" size="5"/></td>
<td><input name="amp_mod_depth_2" type="text" value="%.2f" size="5"/></td>
<td><input name="amp_mod_depth_3" type="text" value="%.2f" size="5"/></td>
</tr>
<tr>
<td><input name="amp_mod_period_1" type="text" value="%.1f" size="5"/>s</td>
<td><input name="amp_mod_period_2" type="text" value="%.1f" size="5"/>s</td>
<td><input name="amp_mod_period_3" type="text" value="%.1f" size="5"/>s</td>
</tr>
<tr>
<td><input name="amp_mod_ratio_1" type="text" value="%.2f" size="5"/></td>
<td><input name="amp_mod_ratio_2" type="text" value="%.2f" size="5"/></td>
<td><input name="amp_mod_ratio_3" type="text" value="%.2f" size="5"/></td>
</tr>
<tr>
<td><input name="amp_mod_offset_1" type="text" value="%.2f" size="5"/></td>
<td><input name="amp_mod_offset_2" type="text" value="%.2f" size="5"/></td>
<td><input name="amp_mod_offset_3" type="text" value="%.2f" size="5"/></td>
</tr>
</table>
</div>
)r";

;

const char* velocity_l = R"r(
Velocity - extra: <input type="checkbox" onclick="javascript:document.getElementById('vm').style.display = this.checked?'block':'none';" style="width: 4vw; height: 4vw;"/>
<table width="100%%">
<tr>
<td><input name="velocity_1" type="text" value="%.1f" size="5"/>p/s</td>
<td><input name="velocity_2" type="text" value="%.1f" size="5"/>p/s</td>
<td><input name="velocity_3" type="text" value="%.1f" size="5"/>p/s</td>
</tr>
</table>
)r";

;

const char* vm_l = R"r(
<div id="vm" style="display: none">
Modulation depth, period, ratio, offset
<table width="100%%">
<tr>
<td><input name="velocity_mod_depth_1" type="text" value="%.2f" size="5"/></td>
<td><input name="velocity_mod_depth_2" type="text" value="%.2f" size="5"/></td>
<td><input name="velocity_mod_depth_3" type="text" value="%.2f" size="5"/></td>
</tr>
<tr>
<td><input name="velocity_mod_period_1" type="text" value="%.1f" size="5"/>s</td>
<td><input name="velocity_mod_period_2" type="text" value="%.1f" size="5"/>s</td>
<td><input name="velocity_mod_period_3" type="text" value="%.1f" size="5"/>s</td>
</tr>
<tr>
<td><input name="velocity_mod_ratio_1" type="text" value="%.1f" size="5"/></td>
<td><input name="velocity_mod_ratio_2" type="text" value="%.1f" size="5"/></td>
<td><input name="velocity_mod_ratio_3" type="text" value="%.1f" size="5"/></td>
</tr>
<tr>
<td><input name="velocity_mod_offset_1" type="text" value="%.2f" size="5"/></td>
<td><input name="velocity_mod_offset_2" type="text" value="%.2f" size="5"/></td>
<td><input name="velocity_mod_offset_3" type="text" value="%.2f" size="5"/></td>
</tr>
</table>
</div>
)r";

;
